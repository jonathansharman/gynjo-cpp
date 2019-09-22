//! @file
//! @copyright See <a href="LICENSE.txt">LICENSE.txt</a>.

#pragma once

#include "logger.hpp"
#include "visitation.hpp"

#include <tl/expected.hpp>

#include <compare>
#include <functional>
#include <memory>
#include <optional>
#include <regex>
#include <string>
#include <variant>
#include <vector>

namespace gynjo {
	namespace tok {
		struct add {
			auto operator<=>(add const&) const = default;
		};
		struct sub {
			auto operator<=>(sub const&) const = default;
		};
		struct mul {
			auto operator<=>(mul const&) const = default;
		};
		struct div {
			auto operator<=>(div const&) const = default;
		};
		struct exp {
			auto operator<=>(exp const&) const = default;
		};
		struct lft {
			auto operator<=>(lft const&) const = default;
		};
		struct rht {
			auto operator<=>(rht const&) const = default;
		};
		struct num {
			double value;
			auto operator<=>(num const&) const = default;
		};
		struct sym {
			std::string name;
			auto operator<=>(sym const&) const = default;
		};

		using token = std::variant<add, sub, mul, div, exp, lft, rht, num, sym>;

		auto to_string(token token) -> std::string {
			using namespace std::string_literals;
			return match(
				token,
				[&](add const&) { return "+"s; },
				[&](sub const&) { return "-"s; },
				[&](mul const&) { return "*"s; },
				[&](div const&) { return "/"s; },
				[&](exp const&) { return "^"s; },
				[&](lft const&) { return "("s; },
				[&](rht const&) { return ")"s; },
				[&](num const& n) { return std::to_string(n.value); },
				[&](sym const& s) { return s.name; });
		}
	}

	auto lex(std::string input) -> tl::expected<std::vector<tok::token>, std::string> {
		using namespace std::string_literals;

		std::vector<std::pair<std::regex, std::function<std::optional<tok::token>(std::smatch const&)>>> map{//
			{std::regex{R"...(\+)..."}, [](std::smatch const&) { return tok::add{}; }},
			{std::regex{R"...(-)..."}, [](std::smatch const&) { return tok::sub{}; }},
			{std::regex{R"...(\*)..."}, [](std::smatch const&) { return tok::mul{}; }},
			{std::regex{R"...(/)..."}, [](std::smatch const&) { return tok::div{}; }},
			{std::regex{R"...(\^)..."}, [](std::smatch const&) { return tok::exp{}; }},
			{std::regex{R"...(\()..."}, [](std::smatch const&) { return tok::lft{}; }},
			{std::regex{R"...(\))..."}, [](std::smatch const&) { return tok::rht{}; }},
			{std::regex{R"...((0|[1-9]\d*)(\.\d+)?)..."},
				[](std::smatch const& m) { return tok::num{std::stod(m.str())}; }},
			{std::regex{R"...([a-zA-Z]+)..."}, [](std::smatch const& m) { return tok::sym{m.str()}; }},
			{std::regex{R"...(\s+)..."}, [](std::smatch const&) { return std::nullopt; }}};

		std::vector<tok::token> result;
		for (auto it = input.cbegin(); it != input.cend();) {
			bool found = false;
			for (auto const& [regex, get_token] : map) {
				std::smatch token_match;
				if (std::regex_search(it, input.cend(), token_match, regex, std::regex_constants::match_continuous)) {
					found = true;
					if (auto const maybe_token = get_token(token_match)) {
						if (maybe_token) { result.push_back(*maybe_token); }
					}
					std::advance(it, token_match.length());
				}
			}
			if (!found) { return tl::unexpected{"unrecognized token"s}; }
		}
		log("lexed [");
		if (!result.empty()) {
			log(to_string(result.front()));
			for (auto it = result.begin() + 1; it != result.end(); ++it) {
				log(" {}", to_string(*it));
			}
		}
		log("]\n");
		return result;
	}
}

#ifndef _DEBUG
#define DOCTEST_CONFIG_DISABLE
#endif
#include <doctest/doctest.h>

TEST_CASE("lexer") {
	using namespace gynjo;

	auto const expected = std::vector<tok::token>{
		tok::num{5}, tok::mul{}, tok::lft{}, tok::num{1}, tok::add{}, tok::num{2}, tok::rht{}};

	auto const actual = lex("5*( 1+	2)");

	CHECK(actual.has_value());
	CHECK(expected == actual.value());
}
