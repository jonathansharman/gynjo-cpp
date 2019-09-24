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

		auto parse_expr(token_it begin, token_it end) -> subparse_result;

		auto parse_atom(token_it begin, token_it end) -> subparse_result {
			if (begin == end) { return tl::unexpected{"expected expression"s}; }
			return match(
				*begin,
				[&](tok::lft const&) -> subparse_result {
					auto expr_result = parse_expr(begin + 1, end);
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
				[&](tok::num const& num) -> subparse_result {
					return std::pair{begin + 1, make_ast(ast::num{num.value})};
				},
				[&](tok::sym const& sym) -> subparse_result {
					return std::pair{begin + 1, make_ast(ast::sym{sym.name})};
				},
				[](auto const& t) -> subparse_result {
					return tl::unexpected{"unexpected token in expression: " + to_string(t)};
				});
		}

		auto parse_exponents(token_it begin, token_it end) -> subparse_result {
			return parse_atom(begin, end);
		}

		auto parse_factors(token_it begin, token_it end) -> subparse_result {
			return parse_exponents(begin, end);
		}

		auto parse_terms(token_it begin, token_it end) -> subparse_result {
			return parse_factors(begin, end).and_then([&](std::pair<token_it, ast::ptr> result) -> subparse_result {
				auto [it, terms] = std::move(result);
				while (it != end && (std::holds_alternative<tok::add>(*it) || std::holds_alternative<tok::sub>(*it))) {
					auto const token = *it;
					auto next_result = parse_factors(it + 1, end);
					if (next_result.has_value()) {
						auto [next_end, next_term] = std::move(next_result.value());
						if (std::holds_alternative<tok::add>(token)) {
							terms = ast::make_ast(ast::add{std::move(terms), std::move(next_term)});
						} else {
							terms = ast::make_ast(ast::sub{std::move(terms), std::move(next_term)});
						}
						it = next_end;
					} else {
						return tl::unexpected{"expected factor"s};
					}
				}
				return std::pair{it, std::move(terms)};
			});
		}

		auto parse_expr(token_it begin, token_it end) -> subparse_result {
			{
				auto result = parse_terms(begin, end);
				if (result.has_value()) { return std::move(result.value()); }
			}
			{
				auto result = parse_factors(begin, end);
				if (result.has_value()) { return std::move(result.value()); }
			}
			{
				auto result = parse_exponents(begin, end);
				if (result.has_value()) { return std::move(result.value()); }
			}
			{
				auto result = parse_atom(begin, end);
				if (result.has_value()) { return std::move(result.value()); }
			}
			return tl::unexpected{"unrecognized expression"s};
		}

		auto parse(token_it begin, token_it end) -> parse_result {
			auto result = parse_expr(begin, end);
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

#ifndef _DEBUG
#define DOCTEST_CONFIG_DISABLE
#endif
#include <doctest/doctest.h>

TEST_CASE("parser") {
	using namespace gynjo;

	auto const expected = //
		make_ast(ast::mul{//
			make_ast(ast::num{5}),
			make_ast(ast::add{//
				make_ast(ast::num{1}),
				make_ast(ast::num{2})})});

	auto const actual = parse(
		std::vector<tok::token>{tok::num{5}, tok::mul{}, tok::lft{}, tok::num{1}, tok::add{}, tok::num{2}, tok::rht{}});

	CHECK(actual.has_value());
	CHECK(to_string(expected) == to_string(actual.value()));
}
