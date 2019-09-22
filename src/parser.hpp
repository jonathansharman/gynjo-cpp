//! @file
//! @copyright See <a href="LICENSE.txt">LICENSE.txt</a>.

#pragma once

#include "lexer.hpp"
#include "visitation.hpp"

namespace gynjo {
	namespace ast {
		using val = std::variant<struct add, struct sub, struct mul, struct div, struct exp, struct num, struct sym>;
		using ptr = std::unique_ptr<val>;

		struct add {
			ptr a;
			ptr b;
		};
		struct sub {
			ptr a;
			ptr b;
		};
		struct mul {
			ptr a;
			ptr b;
		};
		struct div {
			ptr a;
			ptr b;
		};
		struct exp {
			ptr a;
			ptr b;
		};
		struct num {
			double value;
		};
		struct sym {
			std::string name;
		};

		template <typename T>
		auto make_ast(T&& val) {
			return std::make_unique<ast::val>(std::forward<T>(val));
		}

		auto to_string(ptr const& ast) -> std::string {
			return match(
				*ast,
				[](add const& add) { return "(" + to_string(add.a) + " + " + to_string(add.b) + ")"; },
				[](sub const& sub) { return "(" + to_string(sub.a) + " - " + to_string(sub.b) + ")"; },
				[](mul const& mul) { return "(" + to_string(mul.a) + " * " + to_string(mul.b) + ")"; },
				[](div const& div) { return "(" + to_string(div.a) + " / " + to_string(div.b) + ")"; },
				[](exp const& exp) { return "(" + to_string(exp.a) + " ^ " + to_string(exp.b) + ")"; },
				[](num const& num) { return std::to_string(num.value); },
				[](sym const& sym) { return sym.name; });
		}
	}

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
