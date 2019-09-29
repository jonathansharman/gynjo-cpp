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

using namespace std::string_literals;

namespace gynjo {
	namespace {
		template <typename F>
		auto eval_unary(environment& env, ast::ptr const& expr, F&& f) -> eval_result {
			return eval(env, expr).and_then([&](val::value const& val) { return std::forward<F>(f)(val); });
		}

		template <typename F>
		auto eval_binary(environment& env, ast::ptr const& a, ast::ptr const& b, F&& f) -> eval_result {
			return eval(env, a) //
				.and_then([&](val::value const& a) { //
					return eval(env, b) //
						.and_then([&](val::value const& b) { //
							return std::forward<F>(f)(a, b);
						});
				});
		}
	}

	auto product(environment& env, ast::ptr const& factor1, ast::ptr const& factor2) -> eval_result {
		return eval_binary(env, factor1, factor2, [](val::value const& a, val::value const& b) -> eval_result {
			return match2(
				a,
				b,
				[&](val::num const& factor1, val::num const& factor2) -> eval_result {
					return val::num{factor1 * factor2};
				},
				[](auto const& factor1, auto const& factor2) -> eval_result {
					return tl::unexpected{
						fmt::format("cannot multiply {} and {}", val::to_string(factor1), val::to_string(factor2))};
				});
		});
	}

	auto quotient(environment& env, ast::ptr const& dividend, ast::ptr const& divisor) -> eval_result {
		return eval_binary(env, dividend, divisor, [](val::value const& a, val::value const& b) -> eval_result {
			return match2(
				a,
				b,
				[&](val::num const& dividend, val::num const& divisor) -> eval_result {
					if (divisor == 0) { return tl::unexpected{"division by zero"s}; }
					return val::num{dividend / divisor};
				},
				[](auto const& dividend, auto const& divisor) -> eval_result {
					return tl::unexpected{
						fmt::format("cannot divide {} and {}", val::to_string(dividend), val::to_string(divisor))};
				});
		});
	}

	auto power(environment& env, ast::ptr const& base, ast::ptr const& exponent) -> eval_result {
		return eval_binary(env, base, exponent, [](val::value const& a, val::value const& b) -> eval_result {
			return match2(
				a,
				b,
				[&](val::num const& base, val::num const& exponent) -> eval_result {
					return val::num{boost::multiprecision::pow(base, exponent)};
				},
				[](auto const& base, auto const& exponent) -> eval_result {
					return tl::unexpected{
						fmt::format("cannot raise {} to the power of {}", val::to_string(base), val::to_string(exponent))};
				});
		});
	}

	auto application(environment& env, ast::ptr const& f, ast::ptr const& arg) -> eval_result {
		// Evaluate the function and ensure it is a function value.
		return eval(env, f).and_then([&](val::value result) {
			return match(
				result,
				[&](val::fun const& f) {
					// Evaluate the argument.
					return eval(env, arg).and_then([&](val::value arg) {
						return match(
							arg,
							[&](val::tup const& tup) -> eval_result {
								// Ensure correct number of arguments.
								if (tup.elems.size() != f.params.size()) {
									return tl::unexpected{
										fmt::format("attempted to call {}-ary function with {} argument{}.",
											f.params.size(),
											tup.elems.size(),
											tup.elems.size() == 1 ? "" : "s")};
								}
								// Assign arguments to parameters within a local environment.
								auto f_env = env;
								for (std::size_t i = 0; i < tup.elems.size(); ++i) {
									f_env[f.params[i].name] = *tup.elems[i];
								}
								// Evaluate function body within the local environment.
								return eval(f_env, f.body);
							},
							[&](auto const& arg) -> eval_result {
								// Single-argument call.
								if (f.params.size() != 1) {
									return tl::unexpected{
										fmt::format("attempted to call {}-ary function with 1 argument.", f.params.size())};
								}
								// Assign argument to parameter within a local environment.
								auto f_env = env;
								f_env[f.params.front().name] = arg;
								// Evaluate function body within the local environment.
								return eval(f_env, f.body);
							});
					});
				},
				[](auto const& val) -> eval_result {
					return tl::unexpected{fmt::format("attemped to apply {} as a function", val::to_string(val))};
				});
		});
	}

	auto eval(environment& env, ast::ptr const& ast) -> eval_result {
		return match(
			*ast,
			[&](ast::assign const& assign) {
				return eval(env, assign.rhs).and_then([&](val::value expr_value) -> eval_result {
					env[assign.symbol.name] = expr_value;
					return expr_value;
				});
			},
			[&](ast::add const& add) {
				return eval_binary(env, add.addend1, add.addend2, [](val::value const& a, val::value const& b) -> eval_result {
					return match2(
						a,
						b,
						[&](val::num const& addend1, val::num const& addend2) -> eval_result {
							return val::num{addend1 + addend2};
						},
						[](auto const& addend1, auto const& addend2) -> eval_result {
							return tl::unexpected{
								fmt::format("cannot add {} and {}", val::to_string(addend1), val::to_string(addend2))};
						});
				});
			},
			[&](ast::neg const& neg) {
				return eval_unary(env, neg.expr, [](val::value const& val) -> eval_result {
					return match(
						val,
						[&](val::num const& num) -> eval_result { return val::num{-num}; },
						[](auto const& expr) -> eval_result {
							return tl::unexpected{fmt::format("cannot negate {}", to_string(expr))};
						});
				});
			},
			[&](ast::sub const& sub) {
				return eval_binary(env, sub.minuend, sub.subtrahend, [](val::value const& a, val::value const& b) -> eval_result {
					return match2(
						a,
						b,
						[&](val::num const& minuend, val::num const& subtrahend) -> eval_result {
							return val::num{minuend - subtrahend};
						},
						[](auto const& minuend, auto const& subtrahend) -> eval_result {
							return tl::unexpected{fmt::format(
								"cannot subtract {} from {}", val::to_string(subtrahend), val::to_string(minuend))};
						});
				});
			},
			[&](ast::cluster const& cluster_ast) -> eval_result {
				std::vector<val::value> items_val;
				for (auto const& item_ast : cluster_ast.items) {
					auto item_result = eval(env, item_ast);
					if (item_result.has_value()) {
						items_val.push_back(std::move(item_result.value()));
					} else {
						return item_result;
					}
				}
				// Do parenthesized function applications.
				// Do exponentiations.
				// Do non-parenthesized function applications.
				// Do multiplication and division.
				return std::move(items_val.front());
			},
			[&](ast::fun const& f_ast) -> eval_result {
				std::vector<val::fun::param> params_val;
				// The parser guarantees that f_ast.params is tuple of symbols.
				for (auto const& param_ast : std::get<ast::tup>(*f_ast.params).elems) {
					params_val.push_back(val::fun::param{std::get<tok::sym>(*param_ast).name});
				}
				return val::fun{std::move(params_val), clone(*f_ast.body)};
			},
			[&](ast::tup const& tup_ast) -> eval_result {
				val::tup tup_val;
				for (auto const& elem_ast : tup_ast.elems) {
					auto elem_result = eval(env, elem_ast);
					if (elem_result.has_value()) {
						tup_val.elems.push_back(make_value(std::move(elem_result.value())));
					} else {
						return elem_result;
					}
				}
				return tup_val;
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
