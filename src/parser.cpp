//! @file
//! @copyright See <a href="LICENSE.txt">LICENSE.txt</a>.

#include "parser.hpp"

#include "visitation.hpp"

#include <algorithm>

namespace gynjo {
	namespace {
		using namespace std::string_literals;

		//! Parses a function body.
		auto parse_body(token_it begin, token_it end) -> parse_result {
			if (begin == end || !std::holds_alternative<tok::arrow>(*begin)) {
				return tl::unexpected{"expected function body"s};
			}
			return parse_expr(begin + 1, end);
		}

		//! Parses a Gynjo value.
		auto parse_value(token_it begin, token_it end) -> parse_result {
			if (begin == end) { return tl::unexpected{"expected value"s}; }
			auto const first_token = *begin;
			auto it = begin + 1;
			return match(
				first_token,
				// Tuple or lambda
				[&](tok::lparen const&) -> parse_result {
					ast::tup tup;
					// Keep track of whether all tup elements are symbols (possible lambda parameter list).
					bool could_be_lambda = true;
					// Try to parse an expression.
					auto first_result = parse_expr(it, end);
					if (first_result.has_value()) {
						auto [first_end, first] = std::move(first_result.value());
						it = first_end;
						could_be_lambda = could_be_lambda && std::holds_alternative<tok::sym>(first);
						tup.elems->push_back(std::move(first));
						// Try to parse additional comma-delimited expressions.
						while (it != end && std::holds_alternative<tok::com>(*it)) {
							++it;
							auto next_result = parse_expr(it, end);
							if (next_result.has_value()) {
								auto [next_end, next] = std::move(next_result.value());
								it = next_end;
								could_be_lambda = could_be_lambda && std::holds_alternative<tok::sym>(next);
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
							return it_node{body_end, ast::lambda{make_node(std::move(tup)), make_node(std::move(body))}};
						}
					}
					// Collapse singletons back into their contained values. This allows use of parentheses for
					// value grouping without having to special-case interpretation when an argument is a singleton.
					return it_node{it,
						tup.elems->size() == 1
							// Extract singleton element.
							? std::move(tup.elems->front())
							// Return unmodified tuple.
							: std::move(tup)};
				},
				// List
				[&](tok::lsquare const&) -> parse_result {
					ast::list list;
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
					return it_node{it, std::move(list)};
				},
				// Block
				[&](tok::lcurly const&) -> parse_result {
					// Parse statements.
					ast::block block;
					auto stmt_result = parse_stmt(it, end);
					while (stmt_result.has_value()) {
						it = stmt_result.value().it;
						block.stmts->push_back(std::move(stmt_result.value().node));
						stmt_result = parse_stmt(it, end);
					}
					// Parse close curly brace.
					if (it == end || !std::holds_alternative<tok::rcurly>(*it)) {
						return tl::unexpected{"expected '}' after statement block"s};
					}
					++it;
					return it_node{it, std::move(block)};
				},
				// Intrinsic function
				[&](intrinsic f) -> parse_result {
					auto params = [&] {
						switch (f) {
							case intrinsic::top:
								return ast::make_tup(tok::sym{"list"});
							case intrinsic::pop:
								return ast::make_tup(tok::sym{"list"});
							case intrinsic::push:
								return ast::make_tup(tok::sym{"list"}, tok::sym{"value"});
							case intrinsic::print:
								return ast::make_tup(tok::sym{"value"});
							default:
								// unreachable
								return ast::make_tup();
						}
					}();
					return it_node{it, ast::lambda{make_node(std::move(params)), f}};
				},
				// Boolean
				[&](tok::boolean const& b) -> parse_result {
					return it_node{it, b};
				},
				// Number
				[&](tok::num const& num) -> parse_result {
					return it_node{it, num};
				},
				// Symbol or lambda
				[&](tok::sym const& sym) -> parse_result {
					// Could be a parentheses-less unary lambda. Try to parse a lambda body.
					auto body_result = parse_body(it, end);
					if (body_result.has_value()) {
						// Assemble lambda from the parameter wrapped in a tuple and the body.
						auto [body_end, body] = std::move(body_result.value());
						return it_node{body_end, ast::lambda{ast::make_node(ast::make_tup(sym)), make_node(std::move(body))}};
					}
					// It's just a symbol.
					return it_node{it, sym};
				},
				// Anything else is unexpected.
				[](auto const& t) -> parse_result {
					return tl::unexpected{"unexpected token in expression: " + to_string(t)};
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
		auto parse_cluster(token_it begin, token_it end) -> parse_result {
			// Get sign of first item.
			std::vector<bool> negations;
			if (peek_negative(begin, end)) {
				++begin;
				negations.push_back(true);
			} else {
				negations.push_back(false);
			}
			// Parse first item.
			return parse_value(begin, end).and_then([&](it_node result) -> parse_result {
				auto [it, first] = std::move(result);
				auto items = std::make_unique<std::vector<ast::node>>();
				items->push_back(std::move(first));
				// Now parse connectors and subsequent items.
				std::vector<ast::cluster::connector> connectors;
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
							return std::tuple{negative ? 2 : 1, true, ast::cluster::connector::mul};
						},
						[&negations, minus_it = it + 1, end = end](tok::div) {
							bool const negative = peek_negative(minus_it, end);
							negations.push_back(negative);
							// Consume "/" and maybe "-".
							return std::tuple{negative ? 2 : 1, true, ast::cluster::connector::div};
						},
						[&negations, minus_it = it + 1, end = end](tok::exp) {
							bool const negative = peek_negative(minus_it, end);
							negations.push_back(negative);
							// Consume "^" and maybe "-".
							return std::tuple{negative ? 2 : 1, true, ast::cluster::connector::exp};
						},
						[&negations](tok::lparen) {
							negations.push_back(false);
							// Don't consume any tokens.
							return std::tuple{0, true, ast::cluster::connector::adj_paren};
						},
						[&negations](auto const&) {
							negations.push_back(false);
							// Don't consume any tokens.
							return std::tuple{0, false, ast::cluster::connector::adj_nonparen};
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
				return it_node{it,
					items->size() == 1 && negations.front() == false
						// Found a single non-negated value. Just extract it here.
						? std::move(items->front())
						// Found a cluster of values.
						: ast::cluster{std::move(negations), std::move(items), std::move(connectors)}};
			});
		}

		//! Parses a series of additions and subtractions.
		auto parse_terms(token_it begin, token_it end) -> parse_result {
			return parse_cluster(begin, end).and_then([&](it_node result) -> parse_result {
				auto [it, terms] = std::move(result);
				while (it != end && (std::holds_alternative<tok::plus>(*it) || std::holds_alternative<tok::minus>(*it))) {
					// Parse next result.
					auto const token = *it;
					auto next_result = parse_cluster(it + 1, end);
					if (!next_result.has_value()) { return tl::unexpected{"expected term"s}; }
					// Extract result.
					it = next_result.value().it;
					auto next_term = std::move(next_result.value().node);
					// Incorporate into AST.
					if (std::holds_alternative<tok::plus>(token)) {
						terms = ast::add{make_node(std::move(terms)), make_node(std::move(next_term))};
					} else {
						terms = ast::sub{make_node(std::move(terms)), make_node(std::move(next_term))};
					}
				}
				return it_node{it, std::move(terms)};
			});
		}

		//! Parses a series of comparison checks (not including equals or not equals).
		auto parse_comparisons(token_it begin, token_it end) -> parse_result {
			return parse_terms(begin, end).and_then([&](it_node result) -> parse_result {
				auto it = std::move(result.it);
				auto cmps = std::move(result.node);
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
					auto next_cmp = std::move(next_result.value().node);
					// Incorporate into AST.
					match(
						token,
						[&](tok::lt) {
							cmps = ast::lt{make_node(std::move(cmps)), make_node(std::move(next_cmp))};
						},
						[&](tok::leq) {
							cmps = ast::leq{make_node(std::move(cmps)), make_node(std::move(next_cmp))};
						},
						[&](tok::gt) {
							cmps = ast::gt{make_node(std::move(cmps)), make_node(std::move(next_cmp))};
						},
						[&](tok::geq) {
							cmps = ast::geq{make_node(std::move(cmps)), make_node(std::move(next_cmp))};
						},
						[&](auto const&) { /*unreachable*/ });
				}
				return it_node{it, std::move(cmps)};
			});
		}

		//! Parses a series of equality/inequality checks.
		auto parse_eq_checks(token_it begin, token_it end) -> parse_result {
			return parse_comparisons(begin, end).and_then([&](it_node result) -> parse_result {
				auto it = std::move(result.it);
				auto cmps = std::move(result.node);
				auto is_comparison = [](tok::token const& token) {
					return match(
						token,
						[](tok::eq) { return true; },
						[](tok::neq) { return true; },
						[](auto const&) { return false; });
				};
				while (it != end && is_comparison(*it)) {
					// Parse next result.
					auto const token = *it;
					auto next_result = parse_comparisons(it + 1, end);
					if (!next_result.has_value()) { return tl::unexpected{"expected equality check"s}; }
					// Extract result.
					it = std::move(next_result.value().it);
					auto next_cmp = std::move(next_result.value().node);
					// Incorporate into AST.
					match(
						token,
						[&](tok::eq) {
							cmps = ast::eq{make_node(std::move(cmps)), make_node(std::move(next_cmp))};
						},
						[&](tok::neq) {
							cmps = ast::neq{make_node(std::move(cmps)), make_node(std::move(next_cmp))};
						},
						[&](auto const&) { /*unreachable*/ });
				}
				return it_node{it, std::move(cmps)};
			});
		}

		//! Parses a series of logical conjunctions.
		auto parse_conjunctions(token_it begin, token_it end) -> parse_result {
			return parse_eq_checks(begin, end).and_then([&](it_node result) -> parse_result {
				auto it = std::move(result.it);
				auto conjunctions = std::move(result.node);
				while (it != end && std::holds_alternative<tok::and_>(*it)) {
					// Parse next result.
					auto next_result = parse_eq_checks(it + 1, end);
					if (!next_result.has_value()) { return tl::unexpected{"expected conjunction"s}; }
					// Extract result.
					it = std::move(next_result.value().it);
					auto next_conjunction = std::move(next_result.value().node);
					// Incorporate into AST.
					conjunctions = ast::and_{make_node(std::move(conjunctions)), make_node(std::move(next_conjunction))};
				}
				return it_node{it, std::move(conjunctions)};
			});
		}

		//! Parses a series of logical disjunctions.
		auto parse_disjunctions(token_it begin, token_it end) -> parse_result {
			return parse_conjunctions(begin, end).and_then([&](it_node result) -> parse_result {
				auto it = std::move(result.it);
				auto disjunctions = std::move(result.node);
				while (it != end && std::holds_alternative<tok::or_>(*it)) {
					// Store token to match on later.
					auto const token = *it;
					// Parse next result.
					auto next_result = parse_conjunctions(it + 1, end);
					if (!next_result.has_value()) { return tl::unexpected{"expected disjunction"s}; }
					// Extract result.
					it = std::move(next_result.value().it);
					auto next_disjunction = std::move(next_result.value().node);
					// Incorporate into AST.
					disjunctions = ast::or_{make_node(std::move(disjunctions)), make_node(std::move(next_disjunction))};
				}
				return it_node{it, std::move(disjunctions)};
			});
		}

		//! Parses a logical negation. Note that negation is right-associative.
		auto parse_negation(token_it begin, token_it end) -> parse_result {
			if (begin == end) { return tl::unexpected{"expected expression"s}; }
			if (std::holds_alternative<tok::not_>(*begin)) {
				auto neg_result = parse_negation(begin + 1, end);
				if (!neg_result.has_value()) { return tl::unexpected{"expected negation"s}; }
				auto neg_end = std::move(neg_result.value().it);
				auto neg = std::move(neg_result.value().node);
				return it_node{neg_end, ast::not_{make_node(neg)}};
			} else {
				return parse_disjunctions(begin, end);
			}
		}

		//! Parses a return statement, starting after "return".
		auto parse_ret(token_it begin, token_it end) -> parse_result {
			auto ret_result = parse_expr(begin, end);
			if (!ret_result.has_value()) { return tl::unexpected{"expected return expression"s}; }
			return it_node{ret_result.value().it, ast::ret{ast::make_node(std::move(ret_result.value().node))}};
		}

		//! Parses a for-loop, starting after "for".
		auto parse_for_loop(token_it begin, token_it end) -> parse_result {
			if (begin == end) { return tl::unexpected{"expected for-loop"s}; }
			// Parse loop variable.
			return match(
				*begin,
				[&](tok::sym symbol) -> parse_result {
					auto const in_begin = begin + 1;
					// Parse "in".
					if (in_begin == end || !std::holds_alternative<tok::in>(*in_begin)) {
						return tl::unexpected{"expected \"in\" in for-loop"s};
					}
					auto const range_begin = in_begin + 1;
					// Parse range expression.
					return parse_expr(range_begin, end).and_then([&](it_node range_result) -> parse_result {
						auto range_end = range_result.it;
						// Parse "do".
						if (range_end == end || !std::holds_alternative<tok::do_>(*range_end)) {
							return tl::unexpected{"expected \"do\" in for-loop"s};
						}
						auto const body_begin = range_end + 1;
						// Parse body.
						return parse_stmt(body_begin, end).and_then([&](it_node body_result) -> parse_result {
							// Assemble for-loop.
							return it_node{body_result.it,
								ast::for_loop{//
									symbol,
									make_node(std::move(range_result.node)),
									make_node(std::move(body_result.node))}};
						});
					});
				},
				[](auto const&) -> parse_result { return tl::unexpected{"expected assignment symbol"s}; });
		}

		//! Parses a while-loop, starting after "while".
		auto parse_while_loop(token_it begin, token_it end) -> parse_result {
			if (begin == end) { return tl::unexpected{"expected while-loop"s}; }
			// Parse test expression.
			return parse_expr(begin, end).and_then([&](it_node test_result) -> parse_result {
				auto test_end = test_result.it;
				// Parse "do".
				if (test_end == end || !std::holds_alternative<tok::do_>(*test_end)) {
					return tl::unexpected{"expected \"do\" in while-loop"s};
				}
				auto const body_begin = test_end + 1;
				// Parse body.
				return parse_stmt(body_begin, end).and_then([&](it_node body_result) -> parse_result {
					// Assemble while-loop.
					return it_node{body_result.it,
						ast::while_loop{//
							make_node(std::move(test_result.node)),
							make_node(std::move(body_result.node))}};
				});
			});
		}

		//! Parses a branch statment - if-then or if-then-else - starting after "if".
		auto parse_branch(token_it begin, token_it end) -> parse_result {
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
				return it_node{it,
					ast::branch{//
						make_node(std::move(test_result.value().node)),
						make_node(std::move(true_result.value().node)),
						make_node(std::move(false_result.value().node))}};
			} else {
				// Empty else expression.
				return it_node{it,
					ast::branch{//
						make_node(std::move(test_result.value().node)),
						make_node(std::move(true_result.value().node)),
						// Defaults to no-op.
						make_node(ast::nop{})}};
			}
		}

		//! Parses an assignment operation, starting after "let".
		auto parse_assignment(token_it begin, token_it end) -> parse_result {
			if (begin == end) { return tl::unexpected{"expected assignment"s}; }
			// Parse LHS.
			return match(
				*begin,
				[&](tok::sym symbol) -> parse_result {
					auto const eq_begin = begin + 1;
					// Parse "=".
					if (eq_begin == end || !std::holds_alternative<tok::eq>(*(eq_begin))) {
						return tl::unexpected{"expected \"=\" in assignment"s};
					}
					auto const rhs_begin = eq_begin + 1;
					// Parse RHS.
					return parse_expr(rhs_begin, end).and_then([&](it_node rhs_result) -> parse_result {
						// Assemble assignment from symbol and RHS.
						auto [rhs_end, rhs] = std::move(rhs_result);
						return it_node{rhs_end, ast::assign{symbol, make_node(std::move(rhs))}};
					});
				},
				[&](auto const&) -> parse_result {
					return tl::unexpected{"expected loop variable after \"for\", found " + to_string(*begin)};
				});
		}

		//! Parses an import statement, starting after "import".
		auto parse_import(token_it begin, token_it end) -> parse_result {
			if (begin == end) { return tl::unexpected{"expected import target"s}; }
			if (std::holds_alternative<tok::sym>(*begin)) {
				return it_node{begin + 1, ast::imp{std::get<tok::sym>(*begin).name}};
			} else {
				return tl::unexpected{"expected filename"s};
			}
		}
	}

	auto parse_expr(token_it begin, token_it end) -> parse_result {
		return parse_negation(begin, end).and_then([&](it_node result) -> parse_result {
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
			return it_node{it,
				ast::cond{//
					make_node(std::move(result.node)),
					make_node(std::move(true_result.value().node)),
					make_node(std::move(false_result.value().node))}};
		});
	}

	auto parse_stmt(token_it begin, token_it end) -> parse_result {
		// Empty input is a no-op.
		if (begin == end) { return it_node{end, ast::nop{}}; }
		auto stmt_result = match(
			*begin,
			[&](tok::imp) -> parse_result { return parse_import(begin + 1, end); },
			[&](tok::let) -> parse_result { return parse_assignment(begin + 1, end); },
			[&](tok::if_) -> parse_result { return parse_branch(begin + 1, end); },
			[&](tok::while_) -> parse_result { return parse_while_loop(begin + 1, end); },
			[&](tok::for_) -> parse_result { return parse_for_loop(begin + 1, end); },
			[&](tok::ret) -> parse_result { return parse_ret(begin + 1, end); },
			[&](auto const&) -> parse_result {
				return parse_expr(begin, end).and_then([&](it_node result) -> parse_result {
					if (result.it == end || !std::holds_alternative<tok::semicolon>(*result.it)) {
						return tl::unexpected{"missing semicolon after expression statement"s};
					};
					++result.it;
					return result;
				});
			});
		// Check for error in statement.
		if (!stmt_result) { return stmt_result; }
		auto const stmt_end = stmt_result.value().it;
		return it_node{stmt_end, std::move(stmt_result.value().node)};
	}
}
