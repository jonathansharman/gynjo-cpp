//! @file
//! @copyright See <a href="LICENSE.txt">LICENSE.txt</a>.

#pragma once

#include "interpreter.hpp"

#include "ast.hpp"
#include "lexer.hpp"
#include "parser.hpp"
#include "visitation.hpp"

#include <boost/multiprecision/number.hpp>
#include <fmt/format.h>

#include <cmath>

namespace gynjo {
	namespace {
		auto eval(environment& env, ast::ptr const& ast) -> eval_result;

		template <typename F>
		auto eval_unary(environment& env, ast::ptr const& expr, F&& f) -> eval_result {
			return eval(env, expr).and_then([&](val::val const& val) { return std::forward<F>(f)(val); });
		}

		template <typename F>
		auto eval_binary(environment& env, ast::ptr const& a, ast::ptr const& b, F&& f) -> eval_result {
			return eval(env, a) //
				.and_then([&](val::val const& a) { //
					return eval(env, b) //
						.and_then([&](val::val const& b) { //
							return std::forward<F>(f)(a, b);
						});
				});
		}

		auto eval(environment& env, ast::ptr const& ast) -> eval_result {
			using namespace std::string_literals;
			return match(
				*ast,
				[&](ast::assign const& assign) {
					return eval(env, assign.rhs).and_then([&](val::val const& expr_value) -> eval_result {
						env[assign.symbol.name] = expr_value;
						return expr_value;
					});
				},
				[&](ast::add const& add) {
					return eval_binary(env, add.a, add.b, [](val::val const& a, val::val const& b) -> eval_result {
						return match2(a, b, [&](val::num const& a, val::num const& b) { return a + b; });
					});
				},
				[&](ast::neg const& neg) {
					return eval_unary(env, neg.expr, [](val::val const& val) -> eval_result {
						return match(val, [&](val::num const& num) { return -num; });
					});
				},
				[&](ast::sub const& sub) {
					return eval_binary(env, sub.a, sub.b, [](val::val const& a, val::val const& b) -> eval_result {
						return match2(a, b, [&](val::num const& a, val::num const& b) { return a - b; });
					});
				},
				[&](ast::mul const& mul) {
					return eval_binary(env, mul.a, mul.b, [](val::val const& a, val::val const& b) -> eval_result {
						return match2(a, b, [&](val::num const& a, val::num const& b) { return a * b; });
					});
				},
				[&](ast::div const& div) {
					return eval_binary(env, div.a, div.b, [](val::val const& a, val::val const& b) -> eval_result {
						return match2(a, b, [&](val::num const& a, val::num const& b) -> eval_result {
							if (b == 0.0) { return tl::unexpected{"division by zero"s}; }
							return a / b;
						});
					});
				},
				[&](ast::exp const& exp) {
					return eval_binary(env, exp.a, exp.b, [](val::val const& a, val::val const& b) -> eval_result {
						return match2(
							a, b, [&](val::num const& a, val::num const& b) { return boost::multiprecision::pow(a, b); });
					});
				},
				[](tok::num const& num) -> eval_result { return val::num{num.rep}; },
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
