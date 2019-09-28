//! @file
//! @copyright See <a href="LICENSE.txt">LICENSE.txt</a>.

#pragma once

#include "interpreter.hpp"

#ifndef _DEBUG
#define DOCTEST_CONFIG_DISABLE
#endif
#include <doctest/doctest.h>

TEST_SUITE("interpreter") {
	using namespace gynjo;

	TEST_CASE("subtraction") {
		environment env{};
		val::value const expected = val::num{-1.0};
		auto const actual = eval(env, "1-2");
		CHECK(expected == actual.value());
	}

	TEST_CASE("simple compound with parentheses") {
		environment env{};
		val::value const expected = val::num{-15.0};
		auto const actual = eval(env, "-5 * (1 +  2)");
		CHECK(expected == actual.value());
	}

	TEST_CASE("assignment persists") {
		environment env{};
		val::value const expected = val::num{42};
		auto const actual = eval(env, "x = 42");
		CHECK(expected == actual.value());
	}

	TEST_CASE("tuple evaluation") {
		environment env{};
		val::value const expected = val::tup{
			val::make_value(val::num{1}),
			val::make_value(val::tup{
				val::make_value(val::num{2}), val::make_value(val::num{3}) //
			}) //
		};
		auto const actual = eval(env, "(1, (2, 3))");
		CHECK(expected == actual.value());
	}

	TEST_CASE("simple function application") {
		environment env{};
		eval(env, "f = () -> 42");
		val::value const expected = val::num{42};
		auto const actual = eval(env, "f()");
		CHECK(expected == actual.value());
	}

	TEST_CASE("order of operations with function application and implicit multiplication") {
		environment env{};
		eval(env, "a = 2");
		eval(env, "inc = a -> a + 1");
		eval(env, "b = 3");
		val::value const expected = val::num{8};
		auto const actual = eval(env, "a inc b");
		CHECK(expected == actual.value());
	}

	TEST_CASE("higher-order functions") {
		environment env{};
		val::value const expected = val::num{42};
		eval(env, "apply = (f, a) -> f(a)");
		auto const actual = eval(env, "apply(a -> a, 42)");
		CHECK(expected == actual.value());
	}
}
