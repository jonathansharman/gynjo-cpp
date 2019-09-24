//! @file
//! @copyright See <a href="LICENSE.txt">LICENSE.txt</a>.

#pragma once

#include "tokens.hpp"

#include <tl/expected.hpp>

#include <string>
#include <vector>

namespace gynjo {
	using lex_result = tl::expected<std::vector<tok::token>, std::string>;

	auto lex(std::string const& input) -> lex_result;
}
