//! @file
//! @copyright See <a href="LICENSE.txt">LICENSE.txt</a>.

#pragma once

#include "tokens.hpp"

#include <tl/expected.hpp>

#include <string>
#include <vector>

namespace gynjo {
	//! Either a vector of tokens or an error message.
	using lex_result = tl::expected<std::vector<tok::token>, std::string>;

	//! Lexes @p input into a vector of tokens, if possible.
	auto lex(std::string const& input) -> lex_result;
}
