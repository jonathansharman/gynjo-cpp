//! @file
//! @copyright See <a href="LICENSE.txt">LICENSE.txt</a>.

#include "interpreter.hpp"

#include "ast.hpp"
#include "environment.hpp"
#include "lexer.hpp"
#include "parser.hpp"
#include "visitation.hpp"

#include <boost/multiprecision/number.hpp>
#include <fmt/format.h>

#include <cmath>
#include <fstream>

using namespace std::string_literals;

namespace gynjo {
	namespace {
		template <typename F>
		auto eval_unary(environment& env, ast::node const& expr, F&& f) -> eval_result {
			return eval(env, expr).and_then([&](val::value const& val) { return std::forward<F>(f)(val); });
		}

		template <typename F>
		auto eval_binary(environment& env, ast::node const& a, ast::node const& b, F&& f) -> eval_result {
			return eval(env, a) //
				.and_then([&](val::value const& a) { //
					return eval(env, b) //
						.and_then([&](val::value const& b) { //
							return std::forward<F>(f)(a, b);
						});
				});
		}
	}

	auto product(val::value const& dividend, val::value const& divisor) -> eval_result {
		return match2(
			dividend,
			divisor,
			[&](val::num const& factor1, val::num const& factor2) -> eval_result { return factor1 * factor2; },
			[](auto const& factor1, auto const& factor2) -> eval_result {
				return tl::unexpected{
					fmt::format("cannot multiply {} and {}", val::to_string(factor1), val::to_string(factor2))};
			});
	}

	auto quotient(val::value const& dividend, val::value const& divisor) -> eval_result {
		return match2(
			dividend,
			divisor,
			[&](val::num const& dividend, val::num const& divisor) -> eval_result {
				if (divisor == 0) { return tl::unexpected{"division by zero"s}; }
				return dividend / divisor;
			},
			[](auto const& dividend, auto const& divisor) -> eval_result {
				return tl::unexpected{
					fmt::format("cannot divide {} and {}", val::to_string(dividend), val::to_string(divisor))};
			});
	}

	auto power(val::value const& base, val::value const& exponent) -> eval_result {
		return match2(
			base,
			exponent,
			[&](val::num const& base, val::num const& exponent) -> eval_result {
				return boost::multiprecision::pow(base, exponent);
			},
			[](auto const& base, auto const& exponent) -> eval_result {
				return tl::unexpected{
					fmt::format("cannot raise {} to the power of {}", val::to_string(base), val::to_string(exponent))};
			});
	}

	auto application(val::closure const& f, val::tup const& arg) -> eval_result {
		// The parser guarantees the parameter list is a tuple.
		auto const& params = std::get<ast::tup>(*f.lambda.params);
		// Ensure correct number of arguments.
		if (arg.elems->size() != params.elems->size()) {
			return tl::unexpected{fmt::format("attempted to call {}-ary function with {} argument{}.",
				params.elems->size(),
				arg.elems->size(),
				arg.elems->size() == 1 ? "" : "s")};
		}
		// Assign arguments to parameters within a copy of the closure's environment.
		auto app_env = *f.env;
		for (std::size_t i = 0; i < arg.elems->size(); ++i) {
			// The parser guarantees that each parameter is a symbol.
			auto param = std::get<tok::sym>((*params.elems)[i]).name;
			app_env.vars[param] = (*arg.elems)[i];
		}
		// Evaluate function body within the application environment.
		return eval(app_env, *f.lambda.body);
	}

	auto negate(val::value const& value) -> eval_result {
		return match(
			value,
			[](val::num num) -> eval_result { return -num; },
			[](auto const& value) -> eval_result { return tl::unexpected{"cannot negate " + val::to_string(value)}; });
	}

	auto eval(environment& env, ast::node const& node) -> eval_result {
		return match(
			node,
			[](ast::nop) -> eval_result { return val::make_tup(); },
			[&](ast::imp const& imp) -> eval_result {
				std::ifstream fin{imp.filename + ".gynj"};
				if (!fin.is_open()) {
					return tl::unexpected{fmt::format("failed to load library \"{}\"", imp.filename)};
				}
				std::string line;
				while (std::getline(fin, line)) {
					auto line_result = eval(env, line);
					if (!line_result.has_value()) {
						return tl::unexpected{fmt::format("error in \"{}\": {}", imp.filename, line_result.error())};
					}
				}
				return val::make_tup();
			},
			[&](ast::assign const& assign) {
				return eval(env, *assign.rhs).and_then([&](val::value expr_value) -> eval_result {
					env.vars[assign.symbol.name] = expr_value;
					return expr_value;
				});
			},
			[&](ast::add const& add) {
				return eval_binary(env, *add.addend1, *add.addend2, [](val::value const& a, val::value const& b) -> eval_result {
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
			[&](ast::sub const& sub) {
				return eval_binary(env, *sub.minuend, *sub.subtrahend, [](val::value const& a, val::value const& b) -> eval_result {
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
			[&](ast::cluster const& cluster) -> eval_result {
				std::vector<val::value> items;
				for (auto const& item_ast : *(cluster.items)) {
					auto item_result = eval(env, item_ast);
					if (item_result.has_value()) {
						items.push_back(std::move(item_result.value()));
					} else {
						return item_result;
					}
				}
				auto connectors = cluster.connectors;

				// Common functionality of the two function application evaluation loops.
				// Returns an error string if something went wrong or nullopt otherwise.
				auto do_applications = [&](ast::cluster::connector connector) -> std::optional<std::string> {
					for (std::size_t i = 0; i < connectors.size();) {
						if (connectors[i] == connector && std::holds_alternative<val::closure>(items[i])) {
							auto const& f = items[i];
							// Apply negation if necessary.
							if (cluster.negations[i + 1]) {
								auto negate_result = negate(items[i + 1]);
								if (negate_result.has_value()) {
									items[i + 1] = negate_result.value();
								} else {
									return negate_result.error();
								}
							}
							auto const& arg = items[i + 1];
							// Apply function.
							auto result = std::holds_alternative<val::tup>(arg)
								// Argument is already a tuple.
								? application(std::get<val::closure>(f), std::get<val::tup>(arg))
								// Wrap argument in a tuple.
								: application(std::get<val::closure>(f), val::make_tup(arg));
							if (result.has_value()) {
								items[i] = std::move(result.value());
								// Erase consumed item.
								items.erase(items.begin() + i + 1);
								connectors.erase(connectors.begin() + i);
							} else {
								return result.error();
							}
						} else {
							++i;
						}
					}
					return std::nullopt;
				};

				// Do parenthesized function applications.
				if (auto error = do_applications(ast::cluster::connector::adj_paren)) { return tl::unexpected{*error}; }
				// Do exponentiations.
				for (std::size_t i = 0; i < connectors.size();) {
					if (connectors[i] == ast::cluster::connector::exp) {
						auto const& base = items[i];
						// Apply negation if necessary.
						if (cluster.negations[i + 1]) {
							auto negate_result = negate(items[i + 1]);
							if (negate_result.has_value()) {
								items[i + 1] = negate_result.value();
							} else {
								return negate_result;
							}
						}
						auto const& exp = items[i + 1];
						auto result = power(base, exp);
						if (result.has_value()) {
							items[i] = std::move(result.value());
							items.erase(items.begin() + i + 1);
							connectors.erase(connectors.begin() + i);
						} else {
							return result;
						}
					} else {
						++i;
					}
				}
				// Do non-parenthesized function applications.
				if (auto error = do_applications(ast::cluster::connector::adj_nonparen)) {
					return tl::unexpected{*error};
				}
				// Do multiplication and division.
				for (std::size_t i = 0; i < connectors.size();) {
					switch (connectors[i]) {
						case ast::cluster::connector::adj_paren:
							[[fallthrough]];
						case ast::cluster::connector::adj_nonparen:
							[[fallthrough]];
						case ast::cluster::connector::mul: {
							auto const& factor1 = items[i];
							// Apply negation if necessary.
							if (cluster.negations[i + 1]) {
								auto negate_result = negate(items[i + 1]);
								if (negate_result.has_value()) {
									items[i + 1] = negate_result.value();
								} else {
									return negate_result;
								}
							}
							auto const& factor2 = items[i + 1];
							auto result = product(factor1, factor2);
							if (result.has_value()) {
								items[i] = std::move(result.value());
								items.erase(items.begin() + i + 1);
								connectors.erase(connectors.begin() + i);
							} else {
								return result;
							}
							break;
						}
						default: {
							// Division is the only remaining possibility.
							auto const& dividend = items[i];
							// Apply negation if necessary.
							if (cluster.negations[i + 1]) {
								auto negate_result = negate(items[i + 1]);
								if (negate_result.has_value()) {
									items[i + 1] = negate_result.value();
								} else {
									return negate_result;
								}
							}
							auto const& divisor = items[i + 1];
							auto result = quotient(dividend, divisor);
							if (result.has_value()) {
								items[i] = std::move(result.value());
								items.erase(items.begin() + i + 1);
								connectors.erase(connectors.begin() + i);
							} else {
								return result;
							}
						}
					}
				}
				// At this point, all values should be folded into the front of items.
				// Apply final negation if necessary.
				if (cluster.negations.front()) {
					auto negate_result = negate(items.front());
					if (negate_result.has_value()) {
						items.front() = negate_result.value();
					} else {
						return negate_result;
					}
				}
				return items.front();
			},
			[&](ast::lambda const& f) -> eval_result {
				// When a lambda is evaluated, it forms a closure, which includes a copy of the current environment.
				return val::closure{f, std::make_unique<environment>(env)};
			},
			[&](ast::tup const& tup_ast) -> eval_result {
				val::tup tup_val;
				for (auto const& elem_ast : *tup_ast.elems) {
					auto elem_result = eval(env, elem_ast);
					if (elem_result.has_value()) {
						tup_val.elems->push_back(std::move(elem_result.value()));
					} else {
						return elem_result;
					}
				}
				return tup_val;
			},
			[](tok::num const& num) -> eval_result { return val::num{num.rep}; },
			[&](tok::sym const& sym) -> eval_result {
				if (auto it = env.vars.find(sym.name); it != env.vars.end()) {
					return it->second;
				} else {
					return tl::unexpected{fmt::format("'{}' is not defined", sym.name)};
				}
			});
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

	auto print(eval_result result) -> void {
		if (result.has_value()) {
			if (result.value() != val::value{val::make_tup()}) {
				fmt::print("{}\n", gynjo::val::to_string(result.value()));
			}
		} else {
			fmt::print("{}\n", result.error());
		}
	}
}
