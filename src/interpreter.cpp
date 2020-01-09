//! @file
//! @copyright See <a href="LICENSE.txt">LICENSE.txt</a>.

#include "interpreter.hpp"

#include "environment.hpp"
#include "expr.hpp"
#include "lexer.hpp"
#include "parser.hpp"
#include "stmt.hpp"
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
		auto eval_binary(env_ptr const& env, expr const& a, expr const& b, F&& f) -> eval_result {
			return eval(env, a) //
				.and_then([&](val::value const& a) { //
					return eval(env, b) //
						.and_then([&](val::value const& b) { //
							return std::forward<F>(f)(a, b);
						});
				});
		}

		template <typename F>
		auto bin_num_op(env_ptr const& env, val::value const& left, val::value const& right, std::string_view op_name, F&& op)
			-> eval_result {
			return match2(
				left,
				right,
				// Basic
				[&](val::num const& left, val::num const& right) -> eval_result {
					return std::forward<F>(op)(left, right);
				},
				// Empty list
				[](val::empty const&, val::num const&) -> eval_result { return val::empty{}; },
				[](val::num const&, val::empty const&) -> eval_result { return val::empty{}; },
				// Non-empty list
				[&](val::list const& left, val::num const& right) -> eval_result {
					// Perform operation on head.
					auto head_result = bin_num_op(env, *left.head, right, op_name, op);
					if (!head_result.has_value()) { return head_result; }
					// Perform operation on tail.
					auto tail_result = bin_num_op(env, *left.tail, right, op_name, std::forward<F>(op));
					if (!tail_result.has_value()) { return tail_result; }
					// Combine results.
					return val::list{//
						val::make_value(std::move(head_result.value())),
						val::make_value(std::move(tail_result.value()))};
				},
				[&](val::num const& left, val::list const& right) -> eval_result {
					// Perform operation on head.
					auto head_result = bin_num_op(env, left, *right.head, op_name, op);
					if (!head_result.has_value()) { return head_result; }
					// Perform operation on tail.
					auto tail_result = bin_num_op(env, left, *right.tail, op_name, std::forward<F>(op));
					if (!tail_result.has_value()) { return tail_result; }
					// Combine results.
					return val::list{//
						val::make_value(std::move(head_result.value())),
						val::make_value(std::move(tail_result.value()))};
				},
				// Invalid
				[&](auto const&, auto const&) -> eval_result {
					return tl::unexpected{fmt::format(
						"cannot perform {} with {} and {}", op_name, to_string(left, env), to_string(right, env))};
				});
		}

		auto application(val::closure const& c, val::tup const& arg) -> eval_result {
			// The parser guarantees the parameter list is a tuple.
			auto const& params = std::get<tup_expr>(c.f.params->value);
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
				auto param = std::get<tok::sym>((*params.elems)[i].value).name;
				local_env->local_vars[param] = (*arg.elems)[i];
			}
			// Evaluate function body within the application environment.
			return match(
				c.f.body,
				[&](expr_ptr const& body) -> eval_result { return eval(local_env, *body); },
				[&](intrinsic body) -> eval_result {
					switch (body) {
						case intrinsic::top:
							return match(
								*local_env->lookup("list"),
								[](val::list const& list) -> eval_result { return *list.head; },
								[&](auto const& arg) -> eval_result {
									return tl::unexpected{fmt::format(
										"top() expected a non-empty list, found {}", val::to_string(arg, local_env))};
								});
						case intrinsic::pop:
							return match(
								*local_env->lookup("list"),
								[](val::list const& list) -> eval_result { return *list.tail; },
								[&](auto const& arg) -> eval_result {
									return tl::unexpected{fmt::format(
										"pop() expected a non-empty list, found {}", val::to_string(arg, local_env))};
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
								[&](auto const& arg) -> eval_result {
									return tl::unexpected{
										fmt::format("push() expected a list, found {}", val::to_string(arg, local_env))};
								});
						case intrinsic::print:
							std::cout << fmt::format("{}\n", to_string(*local_env->lookup("value"), local_env));
							return val::make_tup();
						case intrinsic::read: {
							std::string result;
							std::getline(std::cin, result);
							return val::value{result};
						}
						default:
							// unreachable
							return tl::unexpected{"call to unknown intrinsic function"s};
					}
				});
		}

		auto negate(env_ptr const& env, val::value const& value) -> eval_result {
			return match(
				value,
				[](val::num num) -> eval_result { return -num; },
				[&](auto const&) -> eval_result { return tl::unexpected{"cannot negate " + to_string(value, env)}; });
		}
	}

	auto eval(env_ptr const& env, expr const& node) -> eval_result {
		return match(
			node.value,
			[&](cond const& cond) {
				return eval(env, *cond.test).and_then([&](val::value const& test_value) -> eval_result {
					return match(
						test_value,
						[&](tok::boolean test) -> eval_result {
							if (test.value) {
								return eval(env, *cond.true_expr);
							} else {
								return eval(env, *cond.false_expr);
							}
						},
						[&](auto const&) -> eval_result {
							return tl::unexpected{
								fmt::format("expected boolean in conditional test, found {}", to_string(test_value, env))};
						});
				});
			},
			[&](block const& block) -> eval_result {
				for (stmt const& stmt : (*block.stmts)) {
					// A return statement exits the block early and produces a value.
					if (std::holds_alternative<ret>(stmt.value)) {
						return eval(env, *std::get<ret>(stmt.value).result);
					}
					// Otherwise, just execute the statement.
					auto stmt_result = exec(env, stmt);
					// Check for error.
					if (!stmt_result.has_value()) {
						return tl::unexpected{"in block statement: " + stmt_result.error()};
					}
				}
				// Return nothing if there was no return statement.
				return val::make_tup();
			},
			[&](and_ const& and_) -> eval_result {
				// Get left.
				auto const left_result = eval(env, *and_.left);
				if (!left_result.has_value()) { return left_result; }
				if (!std::holds_alternative<tok::boolean>(left_result.value())) {
					return tl::unexpected{fmt::format("cannot take logical conjunction of non-boolean value {}",
						to_string(left_result.value(), env))};
				}
				bool const left = std::get<tok::boolean>(left_result.value()).value;
				// Short-circuit if possible.
				if (!left) { return tok::boolean{false}; }
				// Get right.
				auto const right_result = eval(env, *and_.right);
				if (!right_result.has_value()) { return right_result; }
				if (!std::holds_alternative<tok::boolean>(right_result.value())) {
					return tl::unexpected{fmt::format("cannot take logical conjunction of non-boolean value {}",
						to_string(right_result.value(), env))};
				}
				auto const right = std::get<tok::boolean>(right_result.value()).value;
				return tok::boolean{left && right};
			},
			[&](or_ const& or_) -> eval_result {
				// Get left.
				auto const left_result = eval(env, *or_.left);
				if (!left_result.has_value()) { return left_result; }
				if (!std::holds_alternative<tok::boolean>(left_result.value())) {
					return tl::unexpected{fmt::format("cannot take logical disjunction of non-boolean value {}",
						to_string(left_result.value(), env))};
				}
				bool const left = std::get<tok::boolean>(left_result.value()).value;
				// Short-circuit if possible.
				if (left) { return tok::boolean{true}; }
				// Get right.
				auto const right_result = eval(env, *or_.right);
				if (!right_result.has_value()) { return right_result; }
				if (!std::holds_alternative<tok::boolean>(right_result.value())) {
					return tl::unexpected{fmt::format("cannot take logical disjunction of non-boolean value {}",
						to_string(right_result.value(), env))};
				}
				auto const right = std::get<tok::boolean>(right_result.value()).value;
				return tok::boolean{left || right};
			},
			[&](not_ const& not_) {
				return eval(env, *not_.expr).and_then([&](val::value const& val) -> eval_result {
					return match(
						val,
						[](tok::boolean b) -> eval_result { return tok::boolean{!b.value}; },
						[&](auto const&) -> eval_result {
							return tl::unexpected{fmt::format("cannot take logical negation of {}", to_string(val, env))};
						});
				});
			},
			[&](eq const& eq) {
				return eval_binary(env, *eq.left, *eq.right, [](val::value const& left, val::value const& right) -> eval_result {
					return tok::boolean{left == right};
				});
			},
			[&](neq const& neq) {
				return eval_binary(env, *neq.left, *neq.right, [](val::value const& left, val::value const& right) -> eval_result {
					return tok::boolean{left != right};
				});
			},
			[&](approx const& approx) {
				return eval_binary(
					env, *approx.left, *approx.right, [&](val::value const& left, val::value const& right) -> eval_result {
						return tok::boolean{to_string(left, env) == to_string(right, env)};
					});
			},
			[&](lt const& lt) {
				return eval_binary(env, *lt.left, *lt.right, [&](val::value const& a, val::value const& b) -> eval_result {
					return match2(
						a,
						b,
						[&](val::num const& left, val::num const& right) -> eval_result {
							return tok::boolean{left < right};
						},
						[&](auto const&, auto const&) -> eval_result {
							return tl::unexpected{
								fmt::format("cannot compare {} and {}", to_string(a, env), to_string(b, env))};
						});
				});
			},
			[&](leq const& leq) {
				return eval_binary(env, *leq.left, *leq.right, [&](val::value const& a, val::value const& b) -> eval_result {
					return match2(
						a,
						b,
						[&](val::num const& left, val::num const& right) -> eval_result {
							return tok::boolean{left <= right};
						},
						[&](auto const&, auto const&) -> eval_result {
							return tl::unexpected{
								fmt::format("cannot compare {} and {}", to_string(a, env), to_string(b, env))};
						});
				});
			},
			[&](gt const& gt) {
				return eval_binary(env, *gt.left, *gt.right, [&](val::value const& a, val::value const& b) -> eval_result {
					return match2(
						a,
						b,
						[&](val::num const& left, val::num const& right) -> eval_result {
							return tok::boolean{left > right};
						},
						[&](auto const&, auto const&) -> eval_result {
							return tl::unexpected{
								fmt::format("cannot compare {} and {}", to_string(a, env), to_string(b, env))};
						});
				});
			},
			[&](geq const& geq) {
				return eval_binary(env, *geq.left, *geq.right, [&](val::value const& a, val::value const& b) -> eval_result {
					return match2(
						a,
						b,
						[&](val::num const& left, val::num const& right) -> eval_result {
							return tok::boolean{left >= right};
						},
						[&](auto const&, auto const&) -> eval_result {
							return tl::unexpected{
								fmt::format("cannot compare {} and {}", to_string(a, env), to_string(b, env))};
						});
				});
			},
			[&](add const& add) {
				return eval_binary(
					env, *add.addend1, *add.addend2, [&](val::value const& addend1, val::value const& addend2) -> eval_result {
						return bin_num_op(
							env, addend1, addend2, "addition", [](val::num const& addend1, val::num const& addend2) -> eval_result {
								return addend1 + addend2;
							});
					});
			},
			[&](sub const& sub) {
				return eval_binary(
					env, *sub.minuend, *sub.subtrahend, [&](val::value const& minuend, val::value const& subtrahend) -> eval_result {
						return bin_num_op(
							env, minuend, subtrahend, "subtraction", [](val::num const& minuend, val::num const& subtrahend) -> eval_result {
								return minuend - subtrahend;
							});
					});
			},
			[&](cluster const& cluster) -> eval_result {
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
				auto do_applications = [&](cluster::connector connector) -> std::optional<std::string> {
					for (std::size_t i = 0; i < connectors.size();) {
						if (connectors[i] == connector && std::holds_alternative<val::closure>(items[i])) {
							auto const& f = items[i];
							// Apply negation if necessary.
							if (cluster.negations[i + 1]) {
								auto negate_result = negate(env, items[i + 1]);
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
				if (auto error = do_applications(cluster::connector::adj_paren)) { return tl::unexpected{*error}; }
				// Do exponentiations.
				for (std::ptrdiff_t i = connectors.size() - 1; i >= 0; --i) {
					if (connectors[i] == cluster::connector::exp) {
						auto const& base = items[i];
						// Apply negation if necessary.
						if (cluster.negations[i + 1]) {
							auto negate_result = negate(env, items[i + 1]);
							if (negate_result.has_value()) {
								items[i + 1] = negate_result.value();
							} else {
								return negate_result;
							}
						}
						auto const& exp = items[i + 1];
						auto power = bin_num_op(
							env, base, exp, "exponentiation", [](val::num const& base, val::num const& exponent) -> eval_result {
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
						--i;
					}
				}
				// Do non-parenthesized function applications.
				if (auto error = do_applications(cluster::connector::adj_nonparen)) { return tl::unexpected{*error}; }
				// Do multiplication and division.
				for (std::size_t i = 0; i < connectors.size();) {
					switch (connectors[i]) {
						case cluster::connector::adj_paren:
							[[fallthrough]];
						case cluster::connector::adj_nonparen:
							[[fallthrough]];
						case cluster::connector::mul: {
							auto const& factor1 = items[i];
							// Apply negation if necessary.
							if (cluster.negations[i + 1]) {
								auto negate_result = negate(env, items[i + 1]);
								if (negate_result.has_value()) {
									items[i + 1] = negate_result.value();
								} else {
									return negate_result;
								}
							}
							auto const& factor2 = items[i + 1];
							auto product = bin_num_op(
								env, factor1, factor2, "multiplication", [](val::num const& factor1, val::num const& factor2) -> eval_result {
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
								auto negate_result = negate(env, items[i + 1]);
								if (negate_result.has_value()) {
									items[i + 1] = negate_result.value();
								} else {
									return negate_result;
								}
							}
							auto const& divisor = items[i + 1];
							auto quotient = bin_num_op(
								env, dividend, divisor, "division", [](val::num const& dividend, val::num const& divisor) -> eval_result {
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
					auto negate_result = negate(env, items.front());
					if (negate_result.has_value()) {
						items.front() = negate_result.value();
					} else {
						return negate_result;
					}
				}
				return items.front();
			},
			[&](lambda const& f) -> eval_result {
				return val::closure{f, std::make_shared<environment>(env)};
			},
			[&](tup_expr const& tup_expr) -> eval_result {
				val::tup tup;
				for (auto const& elem_expr : *tup_expr.elems) {
					auto elem_result = eval(env, elem_expr);
					if (elem_result.has_value()) {
						tup.elems->push_back(std::move(elem_result.value()));
					} else {
						return elem_result;
					}
				}
				return tup;
			},
			[&](list_expr const& list_expr) -> eval_result {
				val::value list = val::empty{};
				for (auto const& elem_expr : *(list_expr.elems)) {
					auto elem_result = eval(env, elem_expr);
					if (elem_result.has_value()) {
						list = val::list{val::make_value(std::move(elem_result.value())), val::make_value(std::move(list))};
					} else {
						return elem_result;
					}
				}
				return list;
			},
			[](tok::boolean const& b) -> eval_result { return b; },
			[](tok::num const& num) -> eval_result { return val::num{num.rep}; },
			[](std::string const& str) -> eval_result { return str; },
			[&](tok::sym const& sym) -> eval_result {
				if (auto lookup = env->lookup(sym.name)) {
					return *lookup;
				} else {
					return tl::unexpected{fmt::format("'{}' is undefined", sym.name)};
				}
			},
			[&](auto const&) -> eval_result {
				return tl::unexpected{"cannot evaluate imperative statement: " + to_string(node)};
			});
	}

	auto eval(env_ptr const& env, std::string_view input) -> eval_result {
		// Lex.
		lex_result const lex_result = lex(input);
		if (!lex_result.has_value()) { return tl::unexpected{"(lex error) " + lex_result.error()}; }
		auto tokens = std::move(lex_result.value());
		// Parse.
		auto const parse_result = parse_expr(tokens.begin(), tokens.end());
		if (!parse_result.has_value()) { return tl::unexpected{"(parse error) " + parse_result.error()}; }
		auto const expr_end = parse_result.value().it;
		if (expr_end != tokens.end()) {
			return tl::unexpected{"(parse_error) unused tokens starting at " + to_string(*expr_end)};
		}
		// Evaluate.
		return eval(env, parse_result.value().expr);
	}

	auto exec(env_ptr const& env, stmt const& stmt) -> exec_result {
		return match(
			stmt.value,
			[](nop) -> exec_result { return std::monostate(); },
			[&](imp const& imp) -> exec_result {
				std::ifstream fin{imp.filename};
				if (!fin.is_open()) {
					return tl::unexpected{fmt::format("failed to load library \"{}\"", imp.filename)};
				}
				std::stringstream ss;
				ss << fin.rdbuf();
				return exec(env, ss.str());
			},
			[&](assign const& assign) -> exec_result {
				auto rhs_result = eval(env, *assign.rhs);
				// Check for error in RHS.
				if (!rhs_result.has_value()) { return tl::unexpected{"in RHS of assignment: " + rhs_result.error()}; }
				// If the symbol is undefined, initialize it to empty. This allows recursive functions.
				env->local_vars.emplace(assign.symbol.name, val::empty{});
				// Now perform the actual assignment, overwriting whatever's there.
				env->local_vars.insert_or_assign(assign.symbol.name, std::move(rhs_result.value()));
				return std::monostate{};
			},
			[&](branch const& branch) -> exec_result {
				auto test_result = eval(env, *branch.test);
				if (!test_result.has_value()) {
					return tl::unexpected{"in branch test expression: " + test_result.error()};
				}
				return match(
					test_result.value(),
					[&](tok::boolean test) -> exec_result {
						if (test.value) {
							return exec(env, *branch.true_stmt);
						} else {
							return exec(env, *branch.false_stmt);
						}
					},
					[&](auto const&) -> exec_result {
						return tl::unexpected{fmt::format(
							"expected boolean in conditional test, found {}", to_string(test_result.value(), env))};
					});
			},
			[&](while_loop const& loop) -> exec_result {
				for (;;) {
					// Evaluate the test condition.
					auto test_result = eval(env, *loop.test);
					// Check for error in the test expression.
					if (!test_result.has_value()) {
						return tl::unexpected{"in while-loop test expression: " + test_result.error()};
					}
					// Check for non-boolean in the test expression.
					if (!std::holds_alternative<tok::boolean>(test_result.value())) {
						return tl::unexpected{
							"while-loop test value must be boolean, found " + to_string(test_result.value(), env)};
					}
					auto const test = std::get<tok::boolean>(test_result.value());
					if (test.value) {
						// Execute next iteration.
						auto body_result = exec(env, *loop.body);
						// Check for error in body.
						if (!body_result.has_value()) { return body_result; }
					} else {
						// End of loop.
						return std::monostate{};
					}
				}
			},
			[&](for_loop const& loop) -> exec_result {
				auto range_result = eval(env, *loop.range);
				if (!range_result.has_value()) { return tl::unexpected{range_result.error()}; }
				return match(
					range_result.value(),
					[](val::empty) -> exec_result { return std::monostate{}; },
					[&](val::list const& list) -> exec_result {
						auto current = list;
						for (;;) {
							// Assign the loop variable to the current value in the range list.
							env->local_vars[loop.loop_var.name] = *current.head;
							// Execute the loop body in this context.
							auto body_result = exec(env, *loop.body);
							// Check for error in body.
							if (!body_result.has_value()) {
								return tl::unexpected{"in body of for-loop: " + body_result.error()};
							}
							// Iterate.
							if (std::holds_alternative<val::list>(*current.tail)) {
								// Move to the next range element.
								current = std::get<val::list>(*current.tail);
							} else {
								// End of the range.
								break;
							}
						}
						return std::monostate{};
					},
					[&](auto const&) -> exec_result {
						return tl::unexpected{fmt::format("expected a list, found {}", to_string(range_result.value(), env))};
					});
			},
			[&](ret const&) -> exec_result { return tl::unexpected{"cannot return outside statement block"s}; },
			[&](expr_stmt const& expr_stmt) -> exec_result {
				auto eval_result = eval(env, *expr_stmt.expr);
				if (!eval_result.has_value()) { return tl::unexpected{eval_result.error()}; }
				// Expression statements must evaluate to nothing.
				if (eval_result.value() != val::value{val::make_tup()}) {
					return tl::unexpected{"unused expression result: " + to_string(eval_result.value(), env)};
				}
				return std::monostate{};
			});
	}

	auto exec(env_ptr const& env, std::string_view input) -> exec_result {
		// Lex.
		lex_result const lex_result = lex(input);
		if (!lex_result.has_value()) { return tl::unexpected{"(lex error) " + lex_result.error()}; }
		// While there is still input left, parse and execute.
		auto it = lex_result.value().begin();
		auto end = lex_result.value().end();
		while (it != end) {
			// Parse.
			auto const parse_result = parse_stmt(it, end);
			if (!parse_result.has_value()) { return tl::unexpected{"(parse error) " + parse_result.error()}; }
			it = parse_result.value().it;
			// Execute.
			auto exec_result = exec(env, parse_result.value().stmt);
			if (!exec_result.has_value()) { return tl::unexpected{"(runtime error) " + exec_result.error()}; }
		}
		return std::monostate{};
	}
}
