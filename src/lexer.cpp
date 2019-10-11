//! @file
//! @copyright See <a href="LICENSE.txt">LICENSE.txt</a>.

#include "lexer.hpp"

#include "visitation.hpp"

#include <fmt/format.h>

#include <functional>
#include <optional>
#include <regex>

namespace gynjo {
	namespace {
		// For a little brevity...
		using str_to_tok_t = std::function<std::optional<tok::token>(std::string const&)>;
		using sv = std::string_view;
		constexpr auto flags = std::regex::ECMAScript | std::regex::optimize;

		std::initializer_list<std::pair<std::regex, str_to_tok_t>> const regexes_to_tokens = {
			// Whitespace (ignored)
			{std::regex{R"...(\s+)...", flags}, [](sv) { return std::nullopt; }},
			// Comment (ignored)
			{std::regex{R"...(//.*)...", flags}, [](sv) { return std::nullopt; }},
			// Operators/separators
			{std::regex{R"...(=)...", flags}, [](sv) { return tok::eq{}; }},
			{std::regex{R"...(!=)...", flags}, [](sv) { return tok::neq{}; }},
			{std::regex{R"...(<=)...", flags}, [](sv) { return tok::leq{}; }},
			{std::regex{R"...(<)...", flags}, [](sv) { return tok::lt{}; }},
			{std::regex{R"...(>=)...", flags}, [](sv) { return tok::geq{}; }},
			{std::regex{R"...(>)...", flags}, [](sv) { return tok::gt{}; }},
			{std::regex{R"...(\+)...", flags}, [](sv) { return tok::plus{}; }},
			{std::regex{R"...(->)...", flags}, [](sv) { return tok::arrow{}; }},
			{std::regex{R"...(-)...", flags}, [](sv) { return tok::minus{}; }},
			{std::regex{R"...((\*\*)|\^)...", flags}, [](sv) { return tok::exp{}; }},
			{std::regex{R"...(\*)...", flags}, [](sv) { return tok::mul{}; }},
			{std::regex{R"...(/)...", flags}, [](sv) { return tok::div{}; }},
			{std::regex{R"...(\()...", flags}, [](sv) { return tok::lparen{}; }},
			{std::regex{R"...(\))...", flags}, [](sv) { return tok::rparen{}; }},
			{std::regex{R"...(\[)...", flags}, [](sv) { return tok::lsquare{}; }},
			{std::regex{R"...(\])...", flags}, [](sv) { return tok::rsquare{}; }},
			{std::regex{R"...(,)...", flags}, [](sv) { return tok::com{}; }},
			// Value literals
			{std::regex{R"...((\.\d+)|(0|[1-9]\d*)(\.\d+)?)...", flags}, [](sv sv) { return tok::num{sv.data()}; }},
			{std::regex{R"...(true\b)...", flags}, [](sv) { return tok::boolean{true}; }},
			{std::regex{R"...(false\b)...", flags}, [](sv) { return tok::boolean{false}; }},
			// Intrinsic functions
			{std::regex{R"...(top\b)...", flags}, [](sv) { return intrinsic::top; }},
			{std::regex{R"...(pop\b)...", flags}, [](sv) { return intrinsic::pop; }},
			{std::regex{R"...(push\b)...", flags}, [](sv) { return intrinsic::push; }},
			// Keywords
			{std::regex{R"...(import\b)...", flags}, [](sv) { return tok::imp{}; }},
			{std::regex{R"...(let\b)...", flags}, [](sv) { return tok::let{}; }},
			{std::regex{R"...(if\b)...", flags}, [](sv) { return tok::if_{}; }},
			{std::regex{R"...(then\b)...", flags}, [](sv) { return tok::then{}; }},
			{std::regex{R"...(else\b)...", flags}, [](sv) { return tok::else_{}; }},
			{std::regex{R"...(for\b)...", flags}, [](sv) { return tok::for_{}; }},
			{std::regex{R"...(in\b)...", flags}, [](sv) { return tok::in{}; }},
			{std::regex{R"...(do\b)...", flags}, [](sv) { return tok::do_{}; }},
			{std::regex{R"...(and\b)...", flags}, [](sv) { return tok::and_{}; }},
			{std::regex{R"...(or\b)...", flags}, [](sv) { return tok::or_{}; }},
			{std::regex{R"...(not\b)...", flags}, [](sv) { return tok::not_{}; }},
			// Symbol
			{std::regex{R"...([a-zA-Z_]+)...", flags}, [](sv sv) { return tok::sym{sv.data()}; }}};
	}

	auto lex(std::string const& input) -> tl::expected<std::vector<tok::token>, std::string> {
		using namespace std::string_literals;

		std::vector<tok::token> result;
		for (auto it = input.cbegin(); it != input.cend();) {
			bool found = false;
			std::smatch token_match;
			for (auto const& [regex, match_to_token] : regexes_to_tokens) {
				if (std::regex_search(it, input.cend(), token_match, regex, std::regex_constants::match_continuous)) {
					found = true;
					if (auto const maybe_token = match_to_token(token_match.str())) {
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
