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
		using sv = std::string_view;
		using str_to_tok_t = std::function<std::optional<tok::token>(sv)>;
		constexpr auto flags = std::regex::ECMAScript | std::regex::optimize;

		//! Handles the simplest regex/str_to_tok_t cases.
		auto simple(std::string regex, std::optional<tok::token> token) {
			return std::pair{std::regex{regex, flags}, [token](sv) { return token; }};
		}

		//! Handles the reserved-word-based regex/str_to_tok_t cases.
		auto reserved(std::string word, tok::token token) {
			return std::pair{std::regex{word + "(?![a-zA-Z])", flags}, [token](sv) { return token; }};
		}

		std::initializer_list<std::pair<std::regex, str_to_tok_t>> const regexes_to_tokens = {
			// Whitespace (ignored)
			simple(R"...(\s+)...", std::nullopt),
			// Comment (ignored)
			simple(R"...(//.*)...", std::nullopt),
			// Operators/separators
			simple(R"...(=)...", tok::eq{}),
			simple(R"...(!=)...", tok::neq{}),
			simple(R"...(<=)...", tok::leq{}),
			simple(R"...(<)...", tok::lt{}),
			simple(R"...(>=)...", tok::geq{}),
			simple(R"...(>)...", tok::gt{}),
			simple(R"...(\+)...", tok::plus{}),
			simple(R"...(->)...", tok::arrow{}),
			simple(R"...(-)...", tok::minus{}),
			simple(R"...((\*\*)|\^)...", tok::exp{}),
			simple(R"...(\*)...", tok::mul{}),
			simple(R"...(/)...", tok::div{}),
			simple(R"...(\()...", tok::lparen{}),
			simple(R"...(\))...", tok::rparen{}),
			simple(R"...(\[)...", tok::lsquare{}),
			simple(R"...(\])...", tok::rsquare{}),
			simple(R"...(\{)...", tok::lcurly{}),
			simple(R"...(\})...", tok::rcurly{}),
			simple(R"...(,)...", tok::com{}),
			simple(R"...(;)...", tok::semicolon{}),
			simple(R"...(\?)...", tok::que{}),
			simple(R"...(:)...", tok::colon{}),
			// Value literals
			{std::regex{R"...((\.\d+)|(0|[1-9]\d*)(\.\d+)?)...", flags}, [](sv sv) { return tok::num{sv.data()}; }},
			reserved("true", tok::boolean{true}),
			reserved("false", tok::boolean{false}),
			{std::regex{R"...("([^"\\]|\\["\\])*")..."},
				[](sv sv) {
					// Strip quotes and escape characters.
					std::string result;
					for (auto it = sv.begin() + 1; it != sv.end() - 1; ++it) {
						if (*it == '\\') { ++it; }
						result += *it;
					}
					return result;
				}},
			// Intrinsic functions
			reserved("top", intrinsic::top),
			reserved("pop", intrinsic::pop),
			reserved("push", intrinsic::push),
			reserved("print", intrinsic::print),
			reserved("read", intrinsic::read),
			// Keywords
			reserved("import", tok::imp{}),
			reserved("let", tok::let{}),
			reserved("if", tok::if_{}),
			reserved("then", tok::then{}),
			reserved("else", tok::else_{}),
			reserved("while", tok::while_{}),
			reserved("for", tok::for_{}),
			reserved("in", tok::in{}),
			reserved("do", tok::do_{}),
			reserved("return", tok::ret{}),
			reserved("and", tok::and_{}),
			reserved("or", tok::or_{}),
			reserved("not", tok::not_{}),
			// Symbol
			{std::regex{R"...([a-zA-Z_]+)...", flags}, [](sv sv) { return tok::sym{sv.data()}; }}};
	}

	auto lex(std::string_view input) -> tl::expected<std::vector<tok::token>, std::string> {
		using namespace std::string_literals;

		std::vector<tok::token> result;
		for (auto it = input.cbegin(); it != input.cend();) {
			bool found = false;
			std::match_results<std::string_view::const_iterator> token_match;
			// std::smatch token_match;
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
