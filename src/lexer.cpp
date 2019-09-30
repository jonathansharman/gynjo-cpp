//! @file
//! @copyright See <a href="LICENSE.txt">LICENSE.txt</a>.

#include "lexer.hpp"

#include "visitation.hpp"

#include <fmt/format.h>

#include <functional>
#include <optional>
#include <regex>

namespace gynjo {
	auto lex(std::string const& input) -> tl::expected<std::vector<tok::token>, std::string> {
		using namespace std::string_literals;

		using match_to_token = std::function<std::optional<tok::token>(std::smatch const&)>;
		static std::vector<std::pair<std::regex, match_to_token>> map{//
			{std::regex{R"...(//.*)..."}, [](std::smatch const&) { return std::nullopt; }},
			{std::regex{R"...(=)..."}, [](std::smatch const&) { return tok::eq{}; }},
			{std::regex{R"...(\+)..."}, [](std::smatch const&) { return tok::plus{}; }},
			{std::regex{R"...(->)..."}, [](std::smatch const&) { return tok::arrow{}; }},
			{std::regex{R"...(-)..."}, [](std::smatch const&) { return tok::minus{}; }},
			{std::regex{R"...(\*\*)..."}, [](std::smatch const&) { return tok::exp{}; }},
			{std::regex{R"...(\*)..."}, [](std::smatch const&) { return tok::mul{}; }},
			{std::regex{R"...(/)..."}, [](std::smatch const&) { return tok::div{}; }},
			{std::regex{R"...(\^)..."}, [](std::smatch const&) { return tok::exp{}; }},
			{std::regex{R"...(\()..."}, [](std::smatch const&) { return tok::lft{}; }},
			{std::regex{R"...(\))..."}, [](std::smatch const&) { return tok::rht{}; }},
			{std::regex{R"...(,)..."}, [](std::smatch const&) { return tok::com{}; }},
			{std::regex{R"...((\.\d+)|(0|[1-9]\d*)(\.\d+)?)..."}, [](std::smatch const& m) { return tok::num{m.str()}; }},
			{std::regex{R"...([a-zA-Z_]+)..."}, [](std::smatch const& m) { return tok::sym{m.str()}; }},
			{std::regex{R"...(\s+)..."}, [](std::smatch const&) { return std::nullopt; }}};

		std::vector<tok::token> result;
		for (auto it = input.cbegin(); it != input.cend();) {
			bool found = false;
			std::smatch token_match;
			for (auto const& [regex, get_token] : map) {
				if (std::regex_search(it, input.cend(), token_match, regex, std::regex_constants::match_continuous)) {
					found = true;
					if (auto const maybe_token = get_token(token_match)) {
						if (maybe_token) { result.push_back(*maybe_token); }
					}
					std::advance(it, token_match.length());
					break;
				}
			}
			if (!found) {
				std::regex_search(it, input.cend(), token_match, std::regex{R"...(\W+)..."});
				return tl::unexpected{fmt::format("unrecognized token: '{}'", token_match.str())};
			}
		}
		return result;
	}
}
