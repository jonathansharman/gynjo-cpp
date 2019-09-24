//! @file
//! @copyright See <a href="LICENSE.txt">LICENSE.txt</a>.

#pragma once

#include "ast.hpp"
#include "tokens.hpp"

#include <tl/expected.hpp>

#include <string>

namespace gynjo {
	using parse_result = tl::expected<ast::ptr, std::string>;

	auto parse(std::vector<tok::token> tokens) -> parse_result;
}
