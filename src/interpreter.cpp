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
	}

	auto eval(environment& env, ast::ptr const& ast) -> eval_result {
		using namespace std::string_literals;
		return match(
			*ast,
			[&](ast::assign const& assign) {
				return eval(env, assign.rhs).and_then([&](val::val expr_value) -> eval_result {
					env[assign.symbol.name] = expr_value;
					return expr_value;
				});
			},
			[&](ast::add const& add) {
				return eval_binary(env, add.addend1, add.addend2, [](val::val const& a, val::val const& b) -> eval_result {
					return match2(
						a,
						b,
						[&](val::num const& addend1, val::num const& addend2) -> eval_result {
							return addend1 + addend2;
						},
						[](auto const& addend1, auto const& addend2) -> eval_result {
							return tl::unexpected{
								fmt::format("cannot add {} and {}", val::to_string(addend1), val::to_string(addend2))};
						});
				});
			},
			[&](ast::neg const& neg) {
				return eval_unary(env, neg.expr, [](val::val const& val) -> eval_result {
					return match(
						val,
						[&](val::num const& num) -> eval_result { return -num; },
						[](auto const& expr) -> eval_result {
							return tl::unexpected{fmt::format("cannot negate {}", to_string(expr))};
						});
				});
			},
			[&](ast::sub const& sub) {
				return eval_binary(env, sub.minuend, sub.subtrahend, [](val::val const& a, val::val const& b) -> eval_result {
					return match2(
						a,
						b,
						[&](val::num const& minuend, val::num const& subtrahend) -> eval_result {
							return minuend - subtrahend;
						},
						[](auto const& minuend, auto const& subtrahend) -> eval_result {
							return tl::unexpected{fmt::format(
								"cannot subtract {} from {}", val::to_string(subtrahend), val::to_string(minuend))};
						});
				});
			},
			[&](ast::mul const& mul) {
				return eval_binary(env, mul.factor1, mul.factor2, [](val::val const& a, val::val const& b) -> eval_result {
					return match2(
						a,
						b,
						[&](val::num const& factor1, val::num const& factor2) -> eval_result {
							return factor1 * factor2;
						},
						[](auto const& factor1, auto const& factor2) -> eval_result {
							return tl::unexpected{
								fmt::format("cannot multiply {} and {}", val::to_string(factor1), val::to_string(factor2))};
						});
				});
			},
			[&](ast::div const& div) {
				return eval_binary(env, div.dividend, div.divisor, [](val::val const& a, val::val const& b) -> eval_result {
					return match2(
						a,
						b,
						[&](val::num const& dividend, val::num const& divisor) -> eval_result {
							if (divisor == 0) { return tl::unexpected{"division by zero"s}; }
							return dividend / divisor;
						},
						[](auto const& dividend, auto const& divisor) -> eval_result {
							return tl::unexpected{
								fmt::format("cannot divide {} and {}", val::to_string(dividend), val::to_string(divisor))};
						});
				});
			},
			[&](ast::exp const& exp) {
				return eval_binary(env, exp.base, exp.exponent, [](val::val const& a, val::val const& b) -> eval_result {
					return match2(
						a,
						b,
						[&](val::num const& base, val::num const& exponent) -> eval_result {
							return boost::multiprecision::pow(base, exponent);
						},
						[](auto const& base, auto const& exponent) -> eval_result {
							return tl::unexpected{fmt::format(
								"cannot raise {} to the power of {}", val::to_string(base), val::to_string(exponent))};
						});
				});
			},
			[&](ast::app const& app) {
				// Evaluate the function and ensure it is a function value.
				return eval(env, app.f).and_then([&](val::val result) {
					return match(
						result,
						[&](val::fun const& f) {
							// Evaluate the argument and ensure it is a tuple.
							return eval(env, app.arg).and_then([&](val::val arg) {
								return match(
									arg,
									[&](val::tup const& tup) -> eval_result {
										// Ensure correct number of arguments.
										if (tup.elements.size() != f.params.size()) {
											return tl::unexpected{
												fmt::format("attempted to call {}-ary function with {} arguments.",
													f.params.size(),
													tup.elements.size())};
										}
										// Assign arguments to parameters within a local environment.
										auto f_env = env;
										std::size_t const n = tup.elements.size();
										for (std::size_t i = 0; i < n; ++i) {
											f_env[f.params[i].name] = *tup.elements[i];
										}
										// Evaluate function body within the local environment.
										return eval(f_env, f.body);
									},
									[](auto const&) -> eval_result {
										return tl::unexpected{"attempted to call function with non-tuple"s};
									});
							});
						},
						[](auto const& val) -> eval_result {
							return tl::unexpected{fmt::format("attemped to apply {} as a function", val::to_string(val))};
						});
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

	auto eval(environment& env, std::string const& input) -> eval_result {
		lex_result const lex_result = lex(input);
		if (lex_result.has_value()) {
			parse_result const parse_result = parse(env, lex_result.value());
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
