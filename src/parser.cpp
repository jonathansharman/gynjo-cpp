//! @file
//! @copyright See <a href="LICENSE.txt">LICENSE.txt</a>.

#include "parser.hpp"

#include "visitation.hpp"

#include <algorithm>

namespace gynjo {
	namespace {
		using namespace std::string_literals;

		//! Parses a function body.
		auto parse_body(token_it begin, token_it end) -> parse_expr_result {
			if (begin == end || !std::holds_alternative<tok::arrow>(*begin)) {
				return tl::unexpected{"expected function body"s};
			}
			return parse_expr(begin + 1, end);
		}

		//! Parses a Gynjo value.
		auto parse_value(token_it begin, token_it end) -> parse_expr_result {
			if (begin == end) { return tl::unexpected{"expected value"s}; }
			auto const first_token = *begin;
			auto it = begin + 1;
			return match(
				first_token,
				// Tuple or lambda
				[&](tok::lparen const&) -> parse_expr_result {
					tup_expr tup;
					// Keep track of whether all tup elements are symbols (possible lambda parameter list).
					bool could_be_lambda = true;
					// Try to parse an expression.
					auto first_result = parse_expr(it, end);
					if (first_result.has_value()) {
						auto [first_end, first] = std::move(first_result.value());
						it = first_end;
						could_be_lambda = could_be_lambda && std::holds_alternative<tok::sym>(first.value);
						tup.elems->push_back(std::move(first));
						// Try to parse additional comma-delimited expressions.
						while (it != end && std::holds_alternative<tok::com>(*it)) {
							++it;
							auto next_result = parse_expr(it, end);
							if (next_result.has_value()) {
								auto [next_end, next] = std::move(next_result.value());
								it = next_end;
								could_be_lambda = could_be_lambda && std::holds_alternative<tok::sym>(next.value);
								tup.elems->push_back(std::move(next));
							} else {
								return tl::unexpected{"expected expression after ','"s};
							}
						}
					}
					// Parse close parenthesis.
					if (it == end || !std::holds_alternative<tok::rparen>(*it)) {
						return tl::unexpected{"expected ')'"s};
					}
					++it;
					// Check for lambda expression.
					if (could_be_lambda) {
						// Try to parse a lambda body.
						auto body_result = parse_body(it, end);
						if (body_result.has_value()) {
							// Assemble lambda from parameter tuple and body.
							auto [body_end, body] = std::move(body_result.value());
							return it_expr{body_end, lambda{make_expr(std::move(tup)), make_expr(std::move(body))}};
						}
					}
					// Collapse singletons back into their contained values. This allows use of parentheses for
					// value grouping without having to special-case interpretation when an argument is a singleton.
					return it_expr{it,
						tup.elems->size() == 1
							// Extract singleton element.
							? std::move(tup.elems->front())
							// Return unmodified tuple.
							: expr{std::move(tup)}};
				},
				// List
				[&](tok::lsquare const&) -> parse_expr_result {
					list_expr list;
					// Try to parse an expression.
					auto first_result = parse_expr(it, end);
					if (first_result.has_value()) {
						auto [first_end, first] = std::move(first_result.value());
						it = first_end;
						list.elems->push_front(std::move(first));
						// Try to parse additional comma-delimited expressions.
						while (it != end && std::holds_alternative<tok::com>(*it)) {
							++it;
							auto next_result = parse_expr(it, end);
							if (next_result.has_value()) {
								auto [next_end, next] = std::move(next_result.value());
								it = next_end;
								list.elems->push_front(std::move(next));
							} else {
								return tl::unexpected{"expected expression after ',' in list"s};
							}
						}
					}
					// Parse close square bracket.
					if (it == end || !std::holds_alternative<tok::rsquare>(*it)) {
						return tl::unexpected{"expected ']' after list"s};
					}
					++it;
					return it_expr{it, std::move(list)};
				},
				// Block
				[&](tok::lcurly const&) -> parse_expr_result {
					// Parse statements.
					block block;
					auto stmt_result = parse_stmt(it, end);
					while (stmt_result.has_value()) {
						it = stmt_result.value().it;
						block.stmts->push_back(std::move(stmt_result.value().stmt));
						stmt_result = parse_stmt(it, end);
					}
					// Parse close curly brace.
					if (it == end || !std::holds_alternative<tok::rcurly>(*it)) {
						return tl::unexpected{"expected '}' after statement block"s};
					}
					++it;
					return it_expr{it, std::move(block)};
				},
				// Intrinsic function
				[&](intrinsic f) -> parse_expr_result {
					auto params = [&] {
						switch (f) {
							case intrinsic::top:
								return make_tup_expr(expr{tok::sym{"list"}});
							case intrinsic::pop:
								return make_tup_expr(expr{tok::sym{"list"}});
							case intrinsic::push:
								return make_tup_expr(expr{tok::sym{"list"}}, expr{tok::sym{"value"}});
							case intrinsic::print:
								return make_tup_expr(expr{tok::sym{"value"}});
							default:
								// unreachable
								return make_tup_expr();
						}
					}();
					return it_expr{it, lambda{make_expr(std::move(params)), f}};
				},
				// Boolean
				[&](tok::boolean const& b) -> parse_expr_result {
					return it_expr{it, b};
				},
				// Number
				[&](tok::num const& num) -> parse_expr_result {
					return it_expr{it, num};
				},
				// String
				[&](std::string const& str) -> parse_expr_result {
					return it_expr{it, str};
				},
				// Symbol or lambda
				[&](tok::sym const& sym) -> parse_expr_result {
					// Could be a parentheses-less unary lambda. Try to parse a lambda body.
					auto body_result = parse_body(it, end);
					if (body_result.has_value()) {
						// Assemble lambda from the parameter wrapped in a tuple and the body.
						auto [body_end, body] = std::move(body_result.value());
						return it_expr{body_end, lambda{make_expr(make_tup_expr(expr{sym})), make_expr(std::move(body))}};
					}
					// It's just a symbol.
					return it_expr{it, expr{sym}};
				},
				// Anything else is unexpected.
				[](auto const& t) -> parse_expr_result {
					return tl::unexpected{"unexpected token in expression: " + tok::to_string(t)};
				});
		}

		//! Checks whether the next token is a minus.
		auto peek_negative(token_it begin, token_it end) -> bool {
			if (begin == end) { return false; }
			return match(
				*begin,
				// Unary minus
				[&](tok::minus) { return true; },
				// Unary plus (ignored)
				[&](tok::plus) { return false; },
				// Unsigned
				[&](auto const&) { return false; });
		}

		//! Parses a cluster of function calls, exponentiations, (possibly implicit) multiplications, and/or
		//! divisions. The result is something that will require further parsing by the interpreter using
		//! available semantic info.
		auto parse_cluster(token_it begin, token_it end) -> parse_expr_result {
			// Get sign of first item.
			std::vector<bool> negations;
			if (peek_negative(begin, end)) {
				++begin;
				negations.push_back(true);
			} else {
				negations.push_back(false);
			}
			// Parse first item.
			return parse_value(begin, end).and_then([&](it_expr result) -> parse_expr_result {
				auto [it, first] = std::move(result);
				auto items = std::make_unique<std::vector<expr>>();
				items->push_back(std::move(first));
				// Now parse connectors and subsequent items.
				std::vector<cluster::connector> connectors;
				while (it != end) {
					// The following match does three things:
					//   1) Pushes the next negation flag, based on the peeked sign and operator
					//   2) Determines the iterator offset to the start of the next factor
					//   3) Determines Whether the next factor is required or optional
					//   4) Determines the connector to the cluster element, if any
					auto [it_offset, required, connector] = match(
						*it,
						[&negations, minus_it = it + 1, end = end](tok::mul) {
							bool const negative = peek_negative(minus_it, end);
							negations.push_back(negative);
							// Consume "*" and maybe "-".
							return std::tuple{negative ? 2 : 1, true, cluster::connector::mul};
						},
						[&negations, minus_it = it + 1, end = end](tok::div) {
							bool const negative = peek_negative(minus_it, end);
							negations.push_back(negative);
							// Consume "/" and maybe "-".
							return std::tuple{negative ? 2 : 1, true, cluster::connector::div};
						},
						[&negations, minus_it = it + 1, end = end](tok::exp) {
							bool const negative = peek_negative(minus_it, end);
							negations.push_back(negative);
							// Consume "^" and maybe "-".
							return std::tuple{negative ? 2 : 1, true, cluster::connector::exp};
						},
						[&negations](tok::lparen) {
							negations.push_back(false);
							// Don't consume any tokens.
							return std::tuple{0, true, cluster::connector::adj_paren};
						},
						[&negations](auto const&) {
							negations.push_back(false);
							// Don't consume any tokens.
							return std::tuple{0, false, cluster::connector::adj_nonparen};
						});
					// Try to read a cluster element.
					auto next_result = parse_value(it + it_offset, end);
					if (!next_result.has_value()) {
						if (required) {
							// Saw an explicit operator but did not find another cluster element.
							return tl::unexpected{"expected an operand"s};
						} else {
							// This factor was optional. Just stop reading cluster elements now.
							break;
						}
					}
					// Got another cluster item.
					auto [next_end, next_item] = std::move(next_result.value());
					it = next_end;
					items->emplace_back(std::move(next_item));
					connectors.push_back(connector);
				}
				return it_expr{it,
					items->size() == 1 && negations.front() == false
						// Found a single non-negated value. Just extract it here.
						? std::move(items->front())
						// Found a cluster of values.
						: expr{cluster{std::move(negations), std::move(items), std::move(connectors)}}};
			});
		}

		//! Parses a series of additions and subtractions.
		auto parse_terms(token_it begin, token_it end) -> parse_expr_result {
			return parse_cluster(begin, end).and_then([&](it_expr result) -> parse_expr_result {
				auto [it, terms] = std::move(result);
				while (it != end && (std::holds_alternative<tok::plus>(*it) || std::holds_alternative<tok::minus>(*it))) {
					// Parse next result.
					auto const token = *it;
					auto next_result = parse_cluster(it + 1, end);
					if (!next_result.has_value()) { return tl::unexpected{"expected term"s}; }
					// Extract result.
					it = next_result.value().it;
					auto next_term = std::move(next_result.value().expr);
					// Incorporate into expression.
					if (std::holds_alternative<tok::plus>(token)) {
						terms = expr{add{make_expr(std::move(terms)), make_expr(std::move(next_term))}};
					} else {
						terms = expr{sub{make_expr(std::move(terms)), make_expr(std::move(next_term))}};
					}
				}
				return it_expr{it, std::move(terms)};
			});
		}

		//! Parses a series of comparison checks (not including equals or not equals).
		auto parse_comparisons(token_it begin, token_it end) -> parse_expr_result {
			return parse_terms(begin, end).and_then([&](it_expr result) -> parse_expr_result {
				auto it = std::move(result.it);
				auto cmps = std::move(result.expr);
				auto is_comparison = [](tok::token const& token) {
					return match(
						token,
						[](tok::lt) { return true; },
						[](tok::leq) { return true; },
						[](tok::gt) { return true; },
						[](tok::geq) { return true; },
						[](auto const&) { return false; });
				};
				while (it != end && is_comparison(*it)) {
					// Store token to match on later.
					auto const token = *it;
					// Parse next result.
					auto next_result = parse_terms(it + 1, end);
					if (!next_result.has_value()) { return tl::unexpected{"expected comparison"s}; }
					// Extract result.
					it = std::move(next_result.value().it);
					auto next_cmp = std::move(next_result.value().expr);
					// Incorporate into expression.
					match(
						token,
						[&](tok::lt) {
							cmps = expr{lt{make_expr(std::move(cmps)), make_expr(std::move(next_cmp))}};
						},
						[&](tok::leq) {
							cmps = expr{leq{make_expr(std::move(cmps)), make_expr(std::move(next_cmp))}};
						},
						[&](tok::gt) {
							cmps = expr{gt{make_expr(std::move(cmps)), make_expr(std::move(next_cmp))}};
						},
						[&](tok::geq) {
							cmps = expr{geq{make_expr(std::move(cmps)), make_expr(std::move(next_cmp))}};
						},
						[&](auto const&) { /*unreachable*/ });
				}
				return it_expr{it, std::move(cmps)};
			});
		}

		//! Parses a series of equality, inequality, or approximate equality checks.
		auto parse_eq_checks(token_it begin, token_it end) -> parse_expr_result {
			return parse_comparisons(begin, end).and_then([&](it_expr result) -> parse_expr_result {
				auto it = std::move(result.it);
				auto cmps = std::move(result.expr);
				auto is_eq_check = [](tok::token const& token) {
					return match(
						token,
						[](tok::eq) { return true; },
						[](tok::neq) { return true; },
						[](tok::approx) { return true; },
						[](auto const&) { return false; });
				};
				while (it != end && is_eq_check(*it)) {
					// Parse next result.
					auto const token = *it;
					auto next_result = parse_comparisons(it + 1, end);
					if (!next_result.has_value()) { return tl::unexpected{"expected equality check"s}; }
					// Extract result.
					it = std::move(next_result.value().it);
					auto next_cmp = std::move(next_result.value().expr);
					// Incorporate into expression.
					match(
						token,
						[&](tok::eq) {
							cmps = expr{eq{make_expr(std::move(cmps)), make_expr(std::move(next_cmp))}};
						},
						[&](tok::neq) {
							cmps = expr{neq{make_expr(std::move(cmps)), make_expr(std::move(next_cmp))}};
						},
						[&](tok::approx) {
							cmps = expr{approx{make_expr(std::move(cmps)), make_expr(std::move(next_cmp))}};
						},
						[&](auto const&) { /*unreachable*/ });
				}
				return it_expr{it, std::move(cmps)};
			});
		}

		//! Parses a series of logical conjunctions.
		auto parse_conjunctions(token_it begin, token_it end) -> parse_expr_result {
			return parse_eq_checks(begin, end).and_then([&](it_expr result) -> parse_expr_result {
				auto it = std::move(result.it);
				auto conjunctions = std::move(result.expr);
				while (it != end && std::holds_alternative<tok::and_>(*it)) {
					// Parse next result.
					auto next_result = parse_eq_checks(it + 1, end);
					if (!next_result.has_value()) { return tl::unexpected{"expected conjunction"s}; }
					// Extract result.
					it = std::move(next_result.value().it);
					auto next_conjunction = std::move(next_result.value().expr);
					// Incorporate into expression.
					conjunctions = expr{and_{make_expr(std::move(conjunctions)), make_expr(std::move(next_conjunction))}};
				}
				return it_expr{it, std::move(conjunctions)};
			});
		}

		//! Parses a series of logical disjunctions.
		auto parse_disjunctions(token_it begin, token_it end) -> parse_expr_result {
			return parse_conjunctions(begin, end).and_then([&](it_expr result) -> parse_expr_result {
				auto it = std::move(result.it);
				auto disjunctions = std::move(result.expr);
				while (it != end && std::holds_alternative<tok::or_>(*it)) {
					// Store token to match on later.
					auto const token = *it;
					// Parse next result.
					auto next_result = parse_conjunctions(it + 1, end);
					if (!next_result.has_value()) { return tl::unexpected{"expected disjunction"s}; }
					// Extract result.
					it = std::move(next_result.value().it);
					auto next_disjunction = std::move(next_result.value().expr);
					// Incorporate into expression.
					disjunctions = expr{or_{make_expr(std::move(disjunctions)), make_expr(std::move(next_disjunction))}};
				}
				return it_expr{it, std::move(disjunctions)};
			});
		}

		//! Parses a logical negation. Note that negation is right-associative.
		auto parse_negation(token_it begin, token_it end) -> parse_expr_result {
			if (begin == end) { return tl::unexpected{"expected expression"s}; }
			if (std::holds_alternative<tok::not_>(*begin)) {
				auto neg_result = parse_negation(begin + 1, end);
				if (!neg_result.has_value()) { return tl::unexpected{"expected negation"s}; }
				auto neg_end = std::move(neg_result.value().it);
				auto neg = std::move(neg_result.value().expr);
				return it_expr{neg_end, not_{make_expr(neg)}};
			} else {
				return parse_disjunctions(begin, end);
			}
		}

		//! Parses a return statement, starting after "return".
		auto parse_ret(token_it begin, token_it end) -> parse_stmt_result {
			auto ret_result = parse_expr(begin, end);
			if (!ret_result.has_value()) { return tl::unexpected{"expected return expression"s}; }
			return it_stmt{ret_result.value().it, ret{make_expr(std::move(ret_result.value().expr))}};
		}

		//! Parses a for-loop, starting after "for".
		auto parse_for_loop(token_it begin, token_it end) -> parse_stmt_result {
			if (begin == end) { return tl::unexpected{"expected for-loop"s}; }
			// Parse loop variable.
			return match(
				*begin,
				[&](tok::sym symbol) -> parse_stmt_result {
					auto const in_begin = begin + 1;
					// Parse "in".
					if (in_begin == end || !std::holds_alternative<tok::in>(*in_begin)) {
						return tl::unexpected{"expected \"in\" in for-loop"s};
					}
					auto const range_begin = in_begin + 1;
					// Parse range expression.
					return parse_expr(range_begin, end).and_then([&](it_expr range_result) -> parse_stmt_result {
						auto range_end = range_result.it;
						// Parse "do".
						if (range_end == end || !std::holds_alternative<tok::do_>(*range_end)) {
							return tl::unexpected{"expected \"do\" in for-loop"s};
						}
						auto const body_begin = range_end + 1;
						// Parse body.
						return parse_stmt(body_begin, end).and_then([&](it_stmt body_result) -> parse_stmt_result {
							// Assemble for-loop.
							return it_stmt{body_result.it,
								for_loop{//
									symbol,
									make_expr(std::move(range_result.expr)),
									make_stmt(std::move(body_result.stmt))}};
						});
					});
				},
				[](auto const&) -> parse_stmt_result { return tl::unexpected{"expected assignment symbol"s}; });
		}

		//! Parses a while-loop, starting after "while".
		auto parse_while_loop(token_it begin, token_it end) -> parse_stmt_result {
			if (begin == end) { return tl::unexpected{"expected while-loop"s}; }
			// Parse test expression.
			return parse_expr(begin, end).and_then([&](it_expr test_result) -> parse_stmt_result {
				auto test_end = test_result.it;
				// Parse "do".
				if (test_end == end || !std::holds_alternative<tok::do_>(*test_end)) {
					return tl::unexpected{"expected \"do\" in while-loop"s};
				}
				auto const body_begin = test_end + 1;
				// Parse body.
				return parse_stmt(body_begin, end).and_then([&](it_stmt body_result) -> parse_stmt_result {
					// Assemble while-loop.
					return it_stmt{body_result.it,
						while_loop{//
							make_expr(std::move(test_result.expr)),
							make_stmt(std::move(body_result.stmt))}};
				});
			});
		}

		//! Parses a branch statment - if-then or if-then-else - starting after "if".
		auto parse_branch(token_it begin, token_it end) -> parse_stmt_result {
			// Parse test expression.
			auto test_result = parse_expr(begin, end);
			if (!test_result.has_value()) { return tl::unexpected{"expected test expression in branch statement"s}; }
			auto it = test_result.value().it;
			// Parse "then".
			if (it == end || !std::holds_alternative<tok::then>(*it)) {
				return tl::unexpected{"expected \"then\" in branch statement"s};
			}
			++it;
			// Parse statement if true.
			auto true_result = parse_stmt(it, end);
			if (!true_result.has_value()) { return tl::unexpected{"expected true case in branch statement"s}; }
			it = true_result.value().it;
			// Try to parse "else".
			if (it != end && std::holds_alternative<tok::else_>(*it)) {
				++it;
				// Parse statement if false.
				auto false_result = parse_stmt(it, end);
				if (!false_result.has_value()) { return tl::unexpected{"expected false case in branch statement"s}; }
				it = false_result.value().it;
				return it_stmt{it,
					branch{//
						make_expr(std::move(test_result.value().expr)),
						make_stmt(std::move(true_result.value().stmt)),
						make_stmt(std::move(false_result.value().stmt))}};
			} else {
				// Empty else expression.
				return it_stmt{it,
					branch{//
						make_expr(std::move(test_result.value().expr)),
						make_stmt(std::move(true_result.value().stmt)),
						// Defaults to no-op.
						make_stmt(nop{})}};
			}
		}

		//! Parses an assignment operation, starting after "let".
		auto parse_assignment(token_it begin, token_it end) -> parse_stmt_result {
			if (begin == end) { return tl::unexpected{"expected assignment"s}; }
			// Parse LHS.
			return match(
				*begin,
				[&](tok::sym symbol) -> parse_stmt_result {
					auto const eq_begin = begin + 1;
					// Parse "=".
					if (eq_begin == end || !std::holds_alternative<tok::eq>(*(eq_begin))) {
						return tl::unexpected{"expected \"=\" in assignment"s};
					}
					auto const rhs_begin = eq_begin + 1;
					// Parse RHS.
					return parse_expr(rhs_begin, end).and_then([&](it_expr rhs_result) -> parse_stmt_result {
						// Assemble assignment from symbol and RHS.
						auto [rhs_end, rhs] = std::move(rhs_result);
						return it_stmt{rhs_end, assign{symbol, make_expr(std::move(rhs))}};
					});
				},
				[&](auto const&) -> parse_stmt_result {
					return tl::unexpected{"expected loop variable after \"for\", found " + to_string(*begin)};
				});
		}

		//! Parses an import statement, starting after "import".
		auto parse_import(token_it begin, token_it end) -> parse_stmt_result {
			if (begin == end) { return tl::unexpected{"expected import target"s}; }
			return match(
				*begin,
				[&](tok::sym const& filename) -> parse_stmt_result {
					return it_stmt{begin + 1, imp{filename.name}};
				},
				[&](std::string const& filename) -> parse_stmt_result {
					return it_stmt{begin + 1, imp{filename}};
				},
				[&](auto const&) -> parse_stmt_result {
					return tl::unexpected{
						"expected filename (symbol or string) in import statement, found " + to_string(*begin)};
				});
		}
	}

	auto parse_expr(token_it begin, token_it end) -> parse_expr_result {
		return parse_negation(begin, end).and_then([&](it_expr result) -> parse_expr_result {
			auto it = result.it;
			// Check for conditional expression.
			if (result.it == end || !std::holds_alternative<tok::que>(*result.it)) { return result; }
			++it;
			// Parse expression if true.
			auto true_result = parse_expr(it, end);
			if (!true_result.has_value()) { return tl::unexpected{"expected true case in conditional expression"s}; }
			it = true_result.value().it;
			// Parse ":".
			if (it == end || !std::holds_alternative<tok::colon>(*it)) {
				return tl::unexpected{"expected \"?\" in conditional expression"s};
			}
			++it;
			// Parse expression if false.
			auto false_result = parse_expr(it, end);
			if (!false_result.has_value()) { return tl::unexpected{"expected false case in conditional expression"s}; }
			it = false_result.value().it;
			return it_expr{it,
				cond{//
					make_expr(std::move(result.expr)),
					make_expr(std::move(true_result.value().expr)),
					make_expr(std::move(false_result.value().expr))}};
		});
	}

	auto parse_stmt(token_it begin, token_it end) -> parse_stmt_result {
		// Empty input is a no-op.
		if (begin == end) { return it_stmt{end, nop{}}; }
		auto stmt_result = match(
			*begin,
			[&](tok::imp) -> parse_stmt_result { return parse_import(begin + 1, end); },
			[&](tok::let) -> parse_stmt_result { return parse_assignment(begin + 1, end); },
			[&](tok::if_) -> parse_stmt_result { return parse_branch(begin + 1, end); },
			[&](tok::while_) -> parse_stmt_result { return parse_while_loop(begin + 1, end); },
			[&](tok::for_) -> parse_stmt_result { return parse_for_loop(begin + 1, end); },
			[&](tok::ret) -> parse_stmt_result { return parse_ret(begin + 1, end); },
			[&](auto const&) -> parse_stmt_result {
				return parse_expr(begin, end).and_then([&](it_expr result) -> parse_stmt_result {
					if (result.it == end || !std::holds_alternative<tok::semicolon>(*result.it)) {
						return tl::unexpected{"missing semicolon after expression statement"s};
					};
					return it_stmt{result.it + 1, expr_stmt{make_expr(std::move(result.expr))}};
				});
			});
		// Check for error in statement.
		if (!stmt_result) { return stmt_result; }
		auto const stmt_end = stmt_result.value().it;
		return it_stmt{stmt_end, std::move(stmt_result.value().stmt)};
	}
}
