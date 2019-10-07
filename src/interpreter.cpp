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
#include <memory>

using namespace std::string_literals;

namespace gynjo {
	namespace {
		template <typename F>
		auto eval_unary(environment::ptr const& env, ast::node const& expr, F&& f) -> eval_result {
			return eval(env, expr).and_then([&](val::value const val) { return std::forward<F>(f)(val); });
		}

		template <typename F>
		auto eval_binary(environment::ptr const& env, ast::node const& a, ast::node const& b, F&& f) -> eval_result {
			return eval(env, a) //
				.and_then([&](val::value const& a) { //
					return eval(env, b) //
						.and_then([&](val::value const& b) { //
							return std::forward<F>(f)(a, b);
						});
				});
		}

		template <typename F>
		auto bin_num_op(val::value const& left, val::value const& right, std::string_view op_name, F&& op) -> eval_result {
			return match2(
				left,
				right,
				[&](val::num const& left, val::num const& right) -> eval_result {
					return std::forward<F>(op)(left, right);
				},
				[&](auto const&, auto const&) -> eval_result {
					return tl::unexpected{fmt::format(
						"cannot perform {} with {} and {}", op_name, val::to_string(left), val::to_string(right))};
				});
		}

		auto application(val::closure const& c, val::tup const& arg) -> eval_result {
			// The parser guarantees the parameter list is a tuple.
			auto const& params = std::get<ast::tup>(*c.f.params);
			// Ensure correct number of arguments.
			if (arg.elems->size() != params.elems->size()) {
				return tl::unexpected{fmt::format("function requires {} argument{}, received {}",
					params.elems->size(),
					params.elems->size() == 1 ? "" : "s",
					arg.elems->size())};
			}
			// Assign arguments to parameters within a copy of the closure's environment.
			auto local_env = std::make_shared<environment>(c.env);
			for (std::size_t i = 0; i < arg.elems->size(); ++i) {
				// The parser guarantees that each parameter is a symbol.
				auto param = std::get<tok::sym>((*params.elems)[i]).name;
				local_env->local_vars[param] = (*arg.elems)[i];
			}
			// Evaluate function body within the application environment.
			return match(
				c.f.body,
				[&](ast::ptr const& body) -> eval_result { return eval(local_env, *body); },
				[&](intrinsic body) -> eval_result {
					switch (body) {
						case intrinsic::top:
							return match(
								*local_env->lookup("list"),
								[](val::list const& list) -> eval_result { return *list.head; },
								[](auto const& arg) -> eval_result {
									return tl::unexpected{
										fmt::format("top() expected a non-empty list, found {}", val::to_string(arg))};
								});
						case intrinsic::pop:
							return match(
								*local_env->lookup("list"),
								[](val::list const& list) -> eval_result { return *list.tail; },
								[](auto const& arg) -> eval_result {
									return tl::unexpected{
										fmt::format("pop() expected a non-empty list, found {}", val::to_string(arg))};
								});
						case intrinsic::push:
							return match(
								*local_env->lookup("list"),
								[&](val::empty) -> eval_result {
									return val::list{
										val::make_value(*local_env->lookup("value")), val::make_value(val::empty{})};
								},
								[&](val::list const& list) -> eval_result {
									return val::list{val::make_value(*local_env->lookup("value")), val::make_value(list)};
								},
								[](auto const& arg) -> eval_result {
									return tl::unexpected{
										fmt::format("push() expected a list, found {}", val::to_string(arg))};
								});
						default:
							// unreachable
							return tl::unexpected{"call to unknown intrinsic function"s};
					}
				});
		}

		auto negate(val::value const& value) -> eval_result {
			return match(
				value,
				[](val::num num) -> eval_result { return -num; },
				[](auto const& value) -> eval_result { return tl::unexpected{"cannot negate " + val::to_string(value)}; });
		}
	}

	auto eval(environment::ptr const& env, ast::node const& node) -> eval_result {
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
				return eval(env, *assign.rhs).and_then([&](val::value const& expr_value) -> eval_result {
					env->local_vars.emplace(assign.symbol.name, expr_value);
					return expr_value;
				});
			},
			[&](ast::cond const& cond) {
				return eval(env, *cond.test).and_then([&](val::value const& test_value) -> eval_result {
					return match(
						test_value,
						[&](tok::boolean test) -> eval_result {
							if (test.value) {
								return eval(env, *cond.if_true);
							} else {
								return eval(env, *cond.if_false);
							}
						},
						[](auto const& v) -> eval_result {
							return tl::unexpected{
								fmt::format("expected boolean in conditional test, found {}", val::to_string(v))};
						});
				});
			},
			[&](ast::and_ const& and_) -> eval_result {
				// Get left.
				auto const left_result = eval(env, *and_.left);
				if (!left_result.has_value()) { return left_result; }
				if (!std::holds_alternative<tok::boolean>(left_result.value())) {
					return tl::unexpected{fmt::format(
						"cannot take logical conjunction of non-boolean value {}", to_string(left_result.value()))};
				}
				bool const left = std::get<tok::boolean>(left_result.value()).value;
				// Short-circuit if possible.
				if (!left) { return tok::boolean{false}; }
				// Get right.
				auto const right_result = eval(env, *and_.right);
				if (!right_result.has_value()) { return right_result; }
				if (!std::holds_alternative<tok::boolean>(right_result.value())) {
					return tl::unexpected{fmt::format(
						"cannot take logical conjunction of non-boolean value {}", to_string(right_result.value()))};
				}
				auto const right = std::get<tok::boolean>(right_result.value()).value;
				return tok::boolean{left && right};
			},
			[&](ast::or_ const& or_) -> eval_result {
				// Get left.
				auto const left_result = eval(env, *or_.left);
				if (!left_result.has_value()) { return left_result; }
				if (!std::holds_alternative<tok::boolean>(left_result.value())) {
					return tl::unexpected{fmt::format(
						"cannot take logical disjunction of non-boolean value {}", to_string(left_result.value()))};
				}
				bool const left = std::get<tok::boolean>(left_result.value()).value;
				// Short-circuit if possible.
				if (left) { return tok::boolean{true}; }
				// Get right.
				auto const right_result = eval(env, *or_.right);
				if (!right_result.has_value()) { return right_result; }
				if (!std::holds_alternative<tok::boolean>(right_result.value())) {
					return tl::unexpected{fmt::format(
						"cannot take logical disjunction of non-boolean value {}", to_string(right_result.value()))};
				}
				auto const right = std::get<tok::boolean>(right_result.value()).value;
				return tok::boolean{left || right};
			},
			[&](ast::not_ const& not_) {
				return eval_unary(env, *not_.expr, [](val::value const& val) -> eval_result {
					return match(
						val,
						[](tok::boolean b) -> eval_result { return tok::boolean{!b.value}; },
						[](auto const& b) -> eval_result {
							return tl::unexpected{fmt::format("cannot take logical negation of {}", val::to_string(b))};
						});
				});
			},
			[&](ast::eq const& eq) {
				return eval_binary(env, *eq.left, *eq.right, [](val::value const& left, val::value const& right) -> eval_result {
					return tok::boolean{left == right};
				});
			},
			[&](ast::neq const& neq) {
				return eval_binary(env, *neq.left, *neq.right, [](val::value const& left, val::value const& right) -> eval_result {
					return tok::boolean{left != right};
				});
			},
			[&](ast::lt const& lt) {
				return eval_binary(env, *lt.left, *lt.right, [](val::value const& a, val::value const& b) -> eval_result {
					return match2(
						a,
						b,
						[&](val::num const& left, val::num const& right) -> eval_result {
							return tok::boolean{left < right};
						},
						[](auto const& left, auto const& right) -> eval_result {
							return tl::unexpected{
								fmt::format("cannot compare {} and {}", val::to_string(left), val::to_string(right))};
						});
				});
			},
			[&](ast::leq const& leq) {
				return eval_binary(env, *leq.left, *leq.right, [](val::value const& a, val::value const& b) -> eval_result {
					return match2(
						a,
						b,
						[&](val::num const& left, val::num const& right) -> eval_result {
							return tok::boolean{left <= right};
						},
						[](auto const& left, auto const& right) -> eval_result {
							return tl::unexpected{
								fmt::format("cannot compare {} and {}", val::to_string(left), val::to_string(right))};
						});
				});
			},
			[&](ast::gt const& gt) {
				return eval_binary(env, *gt.left, *gt.right, [](val::value const& a, val::value const& b) -> eval_result {
					return match2(
						a,
						b,
						[&](val::num const& left, val::num const& right) -> eval_result {
							return tok::boolean{left > right};
						},
						[](auto const& left, auto const& right) -> eval_result {
							return tl::unexpected{
								fmt::format("cannot compare {} and {}", val::to_string(left), val::to_string(right))};
						});
				});
			},
			[&](ast::geq const& geq) {
				return eval_binary(env, *geq.left, *geq.right, [](val::value const& a, val::value const& b) -> eval_result {
					return match2(
						a,
						b,
						[&](val::num const& left, val::num const& right) -> eval_result {
							return tok::boolean{left >= right};
						},
						[](auto const& left, auto const& right) -> eval_result {
							return tl::unexpected{
								fmt::format("cannot compare {} and {}", val::to_string(left), val::to_string(right))};
						});
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
						auto power = bin_num_op(
							base, exp, "exponentiation", [](val::num const& base, val::num const& exponent) -> eval_result {
								return boost::multiprecision::pow(base, exponent);
							});
						if (power.has_value()) {
							items[i] = std::move(power.value());
							items.erase(items.begin() + i + 1);
							connectors.erase(connectors.begin() + i);
						} else {
							return power;
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
							auto product = bin_num_op(
								factor1, factor2, "multiplication", [](val::num const& factor1, val::num const& factor2) -> eval_result {
									return factor1 * factor2;
								});
							if (product.has_value()) {
								items[i] = std::move(product.value());
								items.erase(items.begin() + i + 1);
								connectors.erase(connectors.begin() + i);
							} else {
								return product;
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
							auto quotient = bin_num_op(
								dividend, divisor, "division", [](val::num const& dividend, val::num const& divisor) -> eval_result {
									if (divisor == 0) { return tl::unexpected{"division by zero"s}; }
									return dividend / divisor;
								});
							if (quotient.has_value()) {
								items[i] = std::move(quotient.value());
								items.erase(items.begin() + i + 1);
								connectors.erase(connectors.begin() + i);
							} else {
								return quotient;
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
				return val::closure{f, std::make_shared<environment>(env)};
			},
			[&](ast::tup const& ast_tup) -> eval_result {
				val::tup val_tup;
				for (auto const& elem_ast : *ast_tup.elems) {
					auto elem_result = eval(env, elem_ast);
					if (elem_result.has_value()) {
						val_tup.elems->push_back(std::move(elem_result.value()));
					} else {
						return elem_result;
					}
				}
				return val_tup;
			},
			[&](ast::list const& ast_list) -> eval_result {
				val::value val_list = val::empty{};
				for (auto const& elem_ast : *(ast_list.elems)) {
					auto elem_result = eval(env, elem_ast);
					if (elem_result.has_value()) {
						val_list = val::list{
							val::make_value(std::move(elem_result.value())), val::make_value(std::move(val_list))};
					} else {
						return elem_result;
					}
				}
				return val_list;
			},
			[](tok::boolean const& b) -> eval_result { return b; },
			[](tok::num const& num) -> eval_result { return val::num{num.rep}; },
			[&](tok::sym const& sym) -> eval_result {
				if (auto lookup = env->lookup(sym.name)) {
					return *lookup;
				} else {
					return tl::unexpected{fmt::format("'{}' is undefined", sym.name)};
				}
			});
	}

	auto eval(environment::ptr const& env, std::string const& input) -> eval_result {
		// Lex.
		lex_result const lex_result = lex(input);
		if (!lex_result.has_value()) { return tl::unexpected{"Lex error: " + lex_result.error()}; }
		// Parse.
		parse_result const parse_result = parse(lex_result.value());
		if (!parse_result.has_value()) { return tl::unexpected{"Parse error: " + parse_result.error()}; }
		// Evaluate.
		return eval(env, parse_result.value());
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
