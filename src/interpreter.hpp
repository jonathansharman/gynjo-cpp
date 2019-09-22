//! @file
//! @copyright See <a href="LICENSE.txt">LICENSE.txt</a>.

#pragma once

#include "parser.hpp"
#include "visitation.hpp"

#include <cmath>

namespace gynjo {
	using eval_result = tl::expected<double, std::string>;

	auto eval(ast::ptr const& ast) -> eval_result;

	template <typename F>
	auto eval_binary(ast::ptr const& a, ast::ptr const& b, F&& f) -> eval_result {
		return eval(a) //
			.and_then([&](double a) { //
				return eval(b) //
					.and_then([&](double b) { //
						return std::forward<F>(f)(a, b);
					});
			});
	}

	auto eval(ast::ptr const& ast) -> eval_result {
		using namespace std::string_literals;
		return match(
			*ast,
			[](ast::add const& add) {
				return eval_binary(add.a, add.b, [](double a, double b) -> eval_result { return a + b; });
			},
			[](ast::sub const& sub) {
				return eval_binary(sub.a, sub.b, [](double a, double b) -> eval_result { return a - b; });
			},
			[](ast::mul const& mul) {
				return eval_binary(mul.a, mul.b, [](double a, double b) -> eval_result { return a * b; });
			},
			[](ast::div const& div) {
				return eval_binary(div.a, div.b, [](double a, double b) -> eval_result {
					if (b == 0.0) return tl::unexpected{"division by zero"s};
					return a / b;
				});
			},
			[](ast::exp const& exp) {
				return eval_binary(exp.a, exp.b, [](double a, double b) -> eval_result { return std::pow(a, b); });
			},
			[](ast::num const& num) -> eval_result { return num.value; },
			[](ast::sym const& sym) -> eval_result {
				(void)sym;
				return 0.0;
			});
	}

	auto eval(std::string input) {
		return lex(input)
			.and_then([](auto const& result) { return parse(result); })
			.and_then([](auto const& result) { return eval(result); })
			.map([](auto const& result) { return std::to_string(result); });
	}
}

#ifndef _DEBUG
#define DOCTEST_CONFIG_DISABLE
#endif
#include <doctest/doctest.h>

TEST_CASE("interpreter") {
	using namespace gynjo;

	double const expected = 15.0;

	auto const actual = eval( //
		make_ast(ast::mul{//
			make_ast(ast::num{5}),
			make_ast(ast::add{//
				make_ast(ast::num{1}),
				make_ast(ast::num{2})})}));

	CHECK(actual.has_value());
	CHECK(expected == doctest::Approx(actual.value()));
}
