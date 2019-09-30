//! @file
//! @copyright See <a href="LICENSE.txt">LICENSE.txt</a>.

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

	TEST_CASE("basic assignment") {
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

	TEST_CASE("order of operations") {
		environment env{};
		eval(env, "inc = a -> a + 1");
		SUBCASE("parenthesized function call before exponentiation") {
			val::value const expected = val::num{36};
			auto const actual = eval(env, "4inc(2)^2");
			CHECK(expected == actual.value());
		}
		SUBCASE("exponentiation before non-parenthesized function call") {
			val::value const expected = val::num{20};
			auto const actual = eval(env, "4inc 2^2");
			CHECK(expected == actual.value());
		}
	}

	TEST_CASE("higher-order functions") {
		environment env{};
		val::value const expected = val::num{42};
		eval(env, "apply = (f, a) -> f(a)");
		auto const actual = eval(env, "apply(a -> a, 42)");
		CHECK(expected == actual.value());
	}

	TEST_CASE("curried function") {
		environment env{};
		val::value const expected = val::num{3};
		eval(env, "sum = a -> b -> a + b");
		auto const actual = eval(env, "sum 1 2");
		CHECK(expected == actual.value());
	}

	TEST_CASE("environment doesn't persist between function chains") {
		environment env{};
		eval(env, "sum = a -> b -> a + b");
		eval(env, "get_a = () -> a");
		auto const result = eval(env, "sum (1) (2) get_a ()");
		// a should be undefined.
		CHECK(!result.has_value());
	}

	TEST_CASE("chained application with and without parentheses") {
		environment env{};
		val::value const expected = val::num{3};
		eval(env, "sum = a -> b -> a + b");
		eval(env, "inc = a -> a + 1");
		auto const actual = eval(env, "sum (1) 2");
		CHECK(expected == actual.value());
	}

	TEST_CASE("chained application does not pollute applications higher in the call chain") {
		environment env{};
		val::value const expected = val::num{8};
		eval(env, "sum = a -> b -> a + b");
		eval(env, "inc = b -> b + 1");
		auto const actual = eval(env, "sum (inc 5) 2");
		CHECK(expected == actual.value());
	}
}
