//! @file
//! @copyright See <a href="LICENSE.txt">LICENSE.txt</a>.

#pragma once

#include "parser.hpp"
#include "visitation.hpp"

#include <cmath>

namespace gynjo {
	using eval_result = std::variant<double, std::string>;

	auto eval(ast::ptr const& ast) -> eval_result;

	template <typename F>
	auto eval_binary(ast::ptr const& a, ast::ptr const& b, F&& f) -> eval_result {
		return match(
			eval(a),
			[](std::string const& error) { return eval_result{error}; },
			[&](double a) {
				return match(
					eval(b),
					[](std::string const& error) { return eval_result{error}; },
					[&](double b) { return eval_result{std::forward<F>(f)(a, b)}; });
			});
	}

	auto eval(ast::ptr const& ast) -> eval_result {
		return match(
			*ast,
			[](ast::add const& add) { return eval_binary(add.a, add.b, [](double a, double b) { return a + b; }); },
			[](ast::sub const& sub) { return eval_binary(sub.a, sub.b, [](double a, double b) { return a - b; }); },
			[](ast::mul const& mul) { return eval_binary(mul.a, mul.b, [](double a, double b) { return a * b; }); },
			[](ast::div const& div) {
				return eval_binary(div.a, div.b, [](double a, double b) { return b == 0.0 ? eval_result{""} : a / b; });
			},
			[](ast::exp const& exp) {
				return eval_binary(exp.a, exp.b, [](double a, double b) { return std::pow(a, b); });
			},
			[](ast::num const& num) { return eval_result{num.value}; },
			[](ast::sym const& sym) {
				(void)sym;
				return eval_result{0.0};
			});
	}

	auto eval(std::string input) {
		return match(
			lex(input),
			[](std::string const& error) { return "Lex error: " + error; },
			[](std::vector<tok::token> const& tokens) {
				return match(
					parse(tokens),
					[](std::string const& error) { return "Parse error: " + error; },
					[](ast::ptr const& ast) {
						return match(
							eval(ast),
							[](std::string const& error) { return "Eval error: " + error; },
							[](double result) { return std::to_string(result); });
					});
			});
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

	CHECK(expected == doctest::Approx(std::get<double>(actual)));
}
