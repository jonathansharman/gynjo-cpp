//! @file
//! @copyright See <a href="LICENSE.txt">LICENSE.txt</a>.

#pragma once

#include "interpreter.hpp"

#include "ast.hpp"
#include "lexer.hpp"
#include "parser.hpp"
#include "visitation.hpp"

#include <fmt/format.h>

#include <cmath>

namespace gynjo {
	namespace {
		auto eval(environment& env, ast::ptr const& ast) -> eval_result;

		template <typename F>
		auto eval_unary(environment& env, ast::ptr const& expr, F&& f) -> eval_result {
			return eval(env, expr).and_then([&](value::val const& value) { return std::forward<F>(f)(value); });
		}

		template <typename F>
		auto eval_binary(environment& env, ast::ptr const& a, ast::ptr const& b, F&& f) -> eval_result {
			return eval(env, a) //
				.and_then([&](value::val const& a) { //
					return eval(env, b) //
						.and_then([&](value::val const& b) { //
							return std::forward<F>(f)(a, b);
						});
				});
		}

		auto eval(environment& env, ast::ptr const& ast) -> eval_result {
			using namespace std::string_literals;
			return match(
				*ast,
				[&](ast::assign const& assign) {
					return eval(env, assign.rhs).and_then([&](value::val const& expr_value) -> eval_result {
						env[assign.symbol.name] = expr_value;
						return expr_value;
					});
				},
				[&](ast::add const& add) {
					return eval_binary(env, add.a, add.b, [](value::val const& a, value::val const& b) -> eval_result {
						return match2(
							a, b, [&](tok::num const& a, tok::num const& b) { return tok::num{a.value + b.value}; });
					});
				},
				[&](ast::neg const& neg) {
					return eval_unary(env, neg.expr, [](value::val const& value) -> eval_result {
						return match(value, [&](tok::num const& num) { return tok::num{-num.value}; });
					});
				},
				[&](ast::sub const& sub) {
					return eval_binary(env, sub.a, sub.b, [](value::val const& a, value::val const& b) -> eval_result {
						return match2(
							a, b, [&](tok::num const& a, tok::num const& b) { return tok::num{a.value - b.value}; });
					});
				},
				[&](ast::mul const& mul) {
					return eval_binary(env, mul.a, mul.b, [](value::val const& a, value::val const& b) -> eval_result {
						return match2(
							a, b, [&](tok::num const& a, tok::num const& b) { return tok::num{a.value * b.value}; });
					});
				},
				[&](ast::div const& div) {
					return eval_binary(env, div.a, div.b, [](value::val const& a, value::val const& b) -> eval_result {
						return match2(a, b, [&](tok::num const& a, tok::num const& b) -> eval_result {
							if (b == tok::num{0.0}) { return tl::unexpected{"division by zero"s}; }
							return tok::num{a.value / b.value};
						});
					});
				},
				[&](ast::exp const& exp) {
					return eval_binary(env, exp.a, exp.b, [](value::val const& a, value::val const& b) -> eval_result {
						return match2(a, b, [&](tok::num const& a, tok::num const& b) {
							return tok::num{std::pow(a.value, b.value)};
						});
					});
				},
				[](tok::num const& num) -> eval_result { return num; },
				[&](tok::sym const& sym) -> eval_result {
					if (auto it = env.find(sym.name); it != env.end()) {
						return it->second;
					} else {
						return tl::unexpected{fmt::format("'{}' is not defined", sym.name)};
					}
				});
		}
	}

	auto eval(environment& env, std::string const& input) -> eval_result {
		lex_result const lex_result = lex(input);
		if (lex_result.has_value()) {
			parse_result const parse_result = parse(lex_result.value());
			if (parse_result.has_value()) {
				return eval(env, parse_result.value());
			} else {
				return tl::unexpected{"Parse error: " + parse_result.error()};
			}
		} else {
			return tl::unexpected{"Lex error: " + lex_result.error()};
		}
	}
}
