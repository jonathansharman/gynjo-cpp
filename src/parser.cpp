//! @file
//! @copyright See <a href="LICENSE.txt">LICENSE.txt</a>.

#include "parser.hpp"

#include "visitation.hpp"

#include <algorithm>

namespace gynjo {
	namespace {
		using namespace std::string_literals;

		using token_it = std::vector<tok::token>::const_iterator;
		using subparse_result = tl::expected<std::pair<token_it, ast::ptr>, std::string>;

		auto parse_terms(token_it begin, token_it end) -> subparse_result;

		//! Parses a function body.
		auto parse_body(token_it begin, token_it end) -> subparse_result {
			if (begin == end || !std::holds_alternative<tok::arrow>(*begin)) {
				return tl::unexpected{"expected function body"s};
			}
			return parse_terms(begin + 1, end);
		}

		//! Parses a Gynjo value.
		auto parse_value(token_it begin, token_it end) -> subparse_result {
			if (begin == end) { return tl::unexpected{"expected value"s}; }
			auto const first_token = *begin;
			auto it = begin + 1;
			return match(
				first_token,
				// Tuple or lambda
				[&](tok::lft const&) -> subparse_result {
					ast::tup tup;
					// Keep track of whether all tup elements are symbols (possible lambda parameter list).
					bool could_be_lambda = true;
					// Try to parse an expression.
					auto first_result = parse_terms(it, end);
					if (first_result.has_value()) {
						auto [first_end, first] = std::move(first_result.value());
						it = first_end;
						could_be_lambda = could_be_lambda && std::holds_alternative<tok::sym>(*first);
						tup.elems.push_back(std::move(first));
						// Try to parse additional comma-delimited expressions.
						while (it != end && std::holds_alternative<tok::com>(*it)) {
							++it;
							auto next_result = parse_terms(it, end);
							if (next_result.has_value()) {
								auto [next_end, next] = std::move(next_result.value());
								it = next_end;
								could_be_lambda = could_be_lambda && std::holds_alternative<tok::sym>(*next);
								tup.elems.push_back(std::move(next));
							} else {
								return tl::unexpected{"expected expression after ','"s};
							}
						}
					}
					// Parse close parenthesis.
					if (it == end || !std::holds_alternative<tok::rht>(*it)) { return tl::unexpected{"expected ')'"s}; }
					++it;
					// Check for lambda expression.
					if (could_be_lambda) {
						// Try to parse a lambda body.
						auto body_result = parse_body(it, end);
						if (body_result.has_value()) {
							// Assemble lambda from parameter tuple and body.
							auto [body_end, body] = std::move(body_result.value());
							return std::pair{body_end, make_node(ast::lambda{make_node(std::move(tup)), std::move(body)})};
						}
					}
					// Collapse singletons back into their contained values. This allows use of parentheses for
					// value grouping without having to special-case interpretation when an argument is a singleton.
					return std::pair{it, tup.elems.size() == 1 ? std::move(tup.elems.front()) : make_node(std::move(tup))};
				},
				// Number
				[&](tok::num const& num) -> subparse_result {
					return std::pair{it, ast::make_node(num)};
				},
				// Symbol or lambda
				[&](tok::sym const& sym) -> subparse_result {
					// Could be a parentheses-less unary lambda. Try to parse a lambda body.
					auto body_result = parse_body(it, end);
					if (body_result.has_value()) {
						// Assemble lambda from the parameter wrapped in a tuple and the body.
						auto [body_end, body] = std::move(body_result.value());
						return std::pair{body_end,
							make_node(ast::lambda{ast::make_node(ast::tup{ast::make_node(sym)}), std::move(body)})};
					}
					// It's just a symbol.
					return std::pair{it, ast::make_node(sym)};
				},
				// Anything else is unexpected.
				[](auto const& t) -> subparse_result {
					return tl::unexpected{"unexpected token in expression: " + to_string(t)};
				});
		}

		//! Parses a cluster of function calls, exponentiations, (possibly implicit) multiplications, and/or divisions.
		//! The result is something that will require further parsing by the interpreter using available semantic info.
		auto parse_cluster(token_it begin, token_it end) -> subparse_result {
			return parse_value(begin, end).and_then([&](std::pair<token_it, ast::ptr> result) -> subparse_result {
				auto [it, first] = std::move(result);
				std::vector<ast::ptr> items;
				items.push_back(std::move(first));
				std::vector<ast::cluster::connector> connectors;
				while (it != end) {
					// The following match determines three things:
					//   1) The iterator offset to the start of the next factor
					//   2) Whether the next factor is required or optional
					//   3) The connector to the cluster element, if any.
					auto [it_offset, required, connector] = match(
						*it,
						[](tok::mul) {
							return std::tuple{1, true, ast::cluster::connector::mul};
						},
						[](tok::div) {
							return std::tuple{1, true, ast::cluster::connector::div};
						},
						[](tok::exp) {
							return std::tuple{1, true, ast::cluster::connector::exp};
						},
						[](tok::lft) {
							return std::tuple{0, true, ast::cluster::connector::adj_paren};
						},
						[](auto const&) {
							return std::tuple{0, false, ast::cluster::connector::adj_nonparen};
						});
					// Try to read a cluster element.
					auto next_result = parse_value(it + it_offset, end);
					if (!next_result.has_value()) {
						if (required) {
							// Saw an explicit operator but did not find another cluster element.
							return tl::unexpected{"expected a value"s};
						} else {
							// This factor was optional. Just stop reading cluster elements now.
							break;
						}
					}
					// Got another cluster item.
					auto [next_end, next_item] = std::move(next_result.value());
					it = next_end;
					items.emplace_back(std::move(next_item));
					connectors.push_back(connector);
				}
				return std::pair{it,
					items.size() == 1
						// Found a single value. Just extract it here.
						? std::move(items.front())
						// Found a cluster of values.
						: make_node(ast::cluster{std::move(items), std::move(connectors)})};
			});
		}

		//! Parses a term along with its leading unary plus or minus, if any.
		auto parse_signed_term(token_it begin, token_it end) -> subparse_result {
			if (begin == end) { return tl::unexpected{"expected (signed) term"s}; }
			return match(
				*begin,
				// Unary minus
				[&](tok::minus) {
					return parse_cluster(begin + 1, end).and_then([&](std::pair<token_it, ast::ptr> result) -> subparse_result {
						auto [factor_end, factor] = std::move(result);
						return std::pair{factor_end, make_node(ast::neg{std::move(factor)})};
					});
				},
				// Unary plus (ignored)
				[&](tok::plus) { return parse_cluster(begin + 1, end); },
				// Unsigned
				[&](auto const&) { return parse_cluster(begin, end); });
		}

		//! Parses a series of additions and subtractions.
		auto parse_terms(token_it begin, token_it end) -> subparse_result {
			return parse_signed_term(begin, end).and_then([&](std::pair<token_it, ast::ptr> result) -> subparse_result {
				auto [it, terms] = std::move(result);
				while (it != end && (std::holds_alternative<tok::plus>(*it) || std::holds_alternative<tok::minus>(*it))) {
					auto const token = *it;
					auto next_result = parse_signed_term(it + 1, end);
					if (next_result.has_value()) {
						auto [next_end, next_term] = std::move(next_result.value());
						it = next_end;
						if (std::holds_alternative<tok::plus>(token)) {
							terms = ast::make_node(ast::add{std::move(terms), std::move(next_term)});
						} else {
							terms = ast::make_node(ast::sub{std::move(terms), std::move(next_term)});
						}
					} else {
						return tl::unexpected{"expected term"s};
					}
				}
				return std::pair{it, std::move(terms)};
			});
		}

		//! Parses an assignment.
		auto parse_assignment(token_it begin, token_it end) -> subparse_result {
			if (begin == end) { return tl::unexpected{"expected assignment"s}; }
			return match(
				*begin,
				// Symbol to assign to
				[&](tok::sym symbol) -> subparse_result {
					auto eq_begin = begin + 1;
					if (eq_begin != end && std::holds_alternative<tok::eq>(*(eq_begin))) {
						// Get RHS.
						return parse_terms(eq_begin + 1, end).and_then([&](std::pair<token_it, ast::ptr> rhs_result) -> subparse_result {
							// Assemble assignment from symbol and RHS.
							auto [rhs_end, rhs] = std::move(rhs_result);
							return std::pair{rhs_end, make_node(ast::assign{symbol, std::move(rhs)})};
						});
					} else {
						return tl::unexpected{"expected '='"s};
					}
				},
				// Otherwise, not a valid assignment
				[](auto const&) -> subparse_result { return tl::unexpected{"expected symbol"s}; });
		}

		//! Parses a statement, which is an assignment or an expression.
		auto parse_statement(token_it begin, token_it end) -> subparse_result {
			// Assignment
			auto assignment_result = parse_assignment(begin, end);
			if (assignment_result.has_value()) { return assignment_result; }
			// Expression
			auto expression_result = parse_terms(begin, end);
			if (expression_result.has_value()) { return expression_result; }
			// Unrecognized statement
			return tl::unexpected{"expected assignment or expression"s};
		}

		//! Parses all tokens from @p begin to @p end, checking that all input is used.
		auto parse(token_it begin, token_it end) -> parse_result {
			auto result = parse_statement(begin, end);
			if (result.has_value()) {
				auto [expr_end, expr] = std::move(result.value());
				if (expr_end != end) {
					return tl::unexpected{"unused input"s};
				} else {
					return std::move(expr);
				}
			} else {
				return tl::unexpected{result.error()};
			}
		}
	}

	auto parse(std::vector<tok::token> tokens) -> parse_result {
		return parse(tokens.begin(), tokens.end());
	}
}
