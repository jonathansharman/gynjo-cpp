//! @file
//! @copyright See <a href="LICENSE.txt">LICENSE.txt</a>.

#pragma once

#include "interpreter.hpp"

#include "ast.hpp"
#include "lexer.hpp"
#include "parser.hpp"
#include "visitation.hpp"

#include <cmath>

namespace gynjo {
	namespace {
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
	}

	auto eval(std::string const& input) -> eval_result {
		lex_result const lex_result = lex(input);
		if (lex_result.has_value()) {
			parse_result const parse_result = parse(lex_result.value());
			if (parse_result.has_value()) {
				return eval(parse_result.value());
			} else {
				return tl::unexpected{"Parse error: " + parse_result.error()};
			}
		} else {
			return tl::unexpected{"Lex error: " + lex_result.error()};
		}
	}
}

#ifndef _DEBUG
#define DOCTEST_CONFIG_DISABLE
#endif
#include <doctest/doctest.h>

TEST_CASE("interpreter") {
	using namespace gynjo;

	double const expected = 15.0;

	auto const actual = eval("5 * (1 +  2)");

	CHECK(actual.has_value());
	CHECK(expected == doctest::Approx(actual.value()));
}
