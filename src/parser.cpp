//! @file
//! @copyright See <a href="LICENSE.txt">LICENSE.txt</a>.

#include "parser.hpp"

#include "lexer.hpp"
#include "logger.hpp"
#include "visitation.hpp"

namespace gynjo {
	namespace {
		using namespace std::string_literals;

		using token_it = std::vector<tok::token>::const_iterator;
		using subparse_result = tl::expected<std::pair<token_it, ast::ptr>, std::string>;

		auto parse_terms(token_it begin, token_it end) -> subparse_result;

		auto parse_atom(token_it begin, token_it end) -> subparse_result {
			if (begin == end) { return tl::unexpected{"expected expression"s}; }
			return match(
				*begin,
				// Parenthetical expression
				[&](tok::lft const&) -> subparse_result {
					auto expr_result = parse_terms(begin + 1, end);
					if (expr_result.has_value()) {
						auto [expr_end, expr] = std::move(expr_result.value());
						if (expr_end == end || !std::holds_alternative<tok::rht>(*expr_end)) {
							return tl::unexpected{"expected ')'"s};
						} else {
							return std::pair{expr_end + 1, std::move(expr)};
						}
					} else {
						return expr_result;
					}
				},
				// Number
				[&](tok::num const& num) -> subparse_result {
					return std::pair{begin + 1, make_ast(ast::val{num})};
				},
				// Symbol
				[&](tok::sym const& sym) -> subparse_result {
					return std::pair{begin + 1, make_ast(ast::val{sym})};
				},
				// Anything else is unexpected.
				[](auto const& t) -> subparse_result {
					return tl::unexpected{"unexpected token in expression: " + to_string(t)};
				});
		}

		auto parse_exponentials(token_it begin, token_it end) -> subparse_result {
			return parse_atom(begin, end).and_then([&](std::pair<token_it, ast::ptr> result) -> subparse_result {
				auto [it, exponentials] = std::move(result);
				while (it != end && std::holds_alternative<tok::exp>(*it)) {
					auto const token = *it;
					auto next_result = parse_atom(it + 1, end);
					if (next_result.has_value()) {
						auto [next_end, next_exponential] = std::move(next_result.value());
						exponentials = ast::make_ast(ast::exp{std::move(exponentials), std::move(next_exponential)});
						it = next_end;
					} else {
						return tl::unexpected{"expected power"s};
					}
				}
				return std::pair{it, std::move(exponentials)};
			});
		}

		auto parse_factors(token_it begin, token_it end) -> subparse_result {
			return parse_exponentials(begin, end).and_then([&](std::pair<token_it, ast::ptr> result) -> subparse_result {
				auto [it, factors] = std::move(result);
				while (it != end) {
					// The following match determines three things:
					//   1) The iterator offset to the start of the next factor
					//   2) Whether the next factor is required or optional
					//   3) Whether to multiply or divide by the next factor
					auto [it_offset, required, multiply] = match(
						*it,
						[](tok::mul) {
							return std::tuple{1, true, true};
						},
						[](tok::div) {
							return std::tuple{1, true, false};
						},
						[](auto const&) {
							return std::tuple{0, false, true};
						});
					// Try to read a factor.
					auto next_result = parse_exponentials(it + it_offset, end);
					if (!next_result.has_value()) {
						if (required) {
							// Saw an explicit operator but did not find another factor.
							return tl::unexpected{"expected factor"s};
						} else {
							// This factor was optional. Just stop reading factors now.
							break;
						}
					}
					// Got another factor.
					auto [next_end, next_factor] = std::move(next_result.value());
					if (multiply) {
						factors = ast::make_ast(ast::mul{std::move(factors), std::move(next_factor)});
					} else {
						factors = ast::make_ast(ast::div{std::move(factors), std::move(next_factor)});
					}
					it = next_end;
				}
				return std::pair{it, std::move(factors)};
			});
		}

		auto parse_signed_term(token_it begin, token_it end) -> subparse_result {
			if (begin == end) { return tl::unexpected{"expected (signed) term"s}; }
			return match(
				*begin,
				// Unary minus
				[&](tok::minus) {
					return parse_factors(begin + 1, end).and_then([&](std::pair<token_it, ast::ptr> result) -> subparse_result {
						auto [factor_end, factor] = std::move(result);
						return std::pair{factor_end, make_ast(ast::neg{std::move(factor)})};
					});
				},
				// Unary plus
				[&](tok::plus) { return parse_factors(begin + 1, end); },
				// Unsigned
				[&](auto const&) { return parse_factors(begin, end); });
		}

		auto parse_terms(token_it begin, token_it end) -> subparse_result {
			return parse_signed_term(begin, end).and_then([&](std::pair<token_it, ast::ptr> result) -> subparse_result {
				auto [it, terms] = std::move(result);
				while (it != end && (std::holds_alternative<tok::plus>(*it) || std::holds_alternative<tok::minus>(*it))) {
					auto const token = *it;
					auto next_result = parse_signed_term(it + 1, end);
					if (next_result.has_value()) {
						auto [next_end, next_term] = std::move(next_result.value());
						if (std::holds_alternative<tok::plus>(token)) {
							terms = ast::make_ast(ast::add{std::move(terms), std::move(next_term)});
						} else {
							terms = ast::make_ast(ast::sub{std::move(terms), std::move(next_term)});
						}
						it = next_end;
					} else {
						return tl::unexpected{"expected term"s};
					}
				}
				return std::pair{it, std::move(terms)};
			});
		}

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
							return std::pair{rhs_end, make_ast(ast::assign{symbol, std::move(rhs)})};
						});
					} else {
						return tl::unexpected{"expected '='"s};
					}
				},
				// Otherwise, not a valid assignment
				[](auto const&) -> subparse_result { return tl::unexpected{"expected symbol"s}; });
		}

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

		auto parse(token_it begin, token_it end) -> parse_result {
			auto result = parse_statement(begin, end);
			if (result.has_value()) {
				auto [expr_end, expr] = std::move(result.value());
				log("parsed {}\n", to_string(expr));
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
