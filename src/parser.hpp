//! @file
//! @copyright See <a href="LICENSE.txt">LICENSE.txt</a>.

#pragma once

#include "ast.hpp"
#include "lexer.hpp"
#include "visitation.hpp"

namespace gynjo {
	using parse_result = tl::expected<ast::ptr, std::string>;
	using token_it = std::vector<tok::token>::const_iterator;

	auto parse_expr(token_it begin, token_it end) -> parse_result {
		using namespace std::string_literals;
		if (begin == end) { return tl::unexpected{"expected expression"s}; }
		return match(
			*begin,
			[&](tok::lft const&) -> parse_result {
				auto expr = parse_expr(begin + 1, end);
				return expr;
			},
			[](tok::num const& num) -> parse_result { return make_ast(ast::num{num.value}); },
			[](tok::sym const& sym) -> parse_result { return make_ast(ast::sym{sym.name}); },
			[](auto const& t) -> parse_result {
				return tl::unexpected{"unexpected token in expression: " + to_string(t)};
			});
	}

	auto parse(std::vector<tok::token> tokens) -> parse_result {
		auto result = parse_expr(tokens.begin(), tokens.end());
		if (result.has_value()) { log("parsed {}\n", to_string(result.value())); }
		return result;
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
