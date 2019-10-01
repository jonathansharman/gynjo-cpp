//! @file
//! @copyright See <a href="LICENSE.txt">LICENSE.txt</a>.

#pragma once

#include "ast.hpp"
#include "tokens.hpp"
#include "values.hpp"

#include <tl/expected.hpp>

#include <string>
#include <vector>

namespace gynjo {
	//! Either an AST pointer or an error message.
	using parse_result = tl::expected<ast::node, std::string>;

	//! Parses @p tokens into an AST in the context of @p env, if possible.
	auto parse(std::vector<tok::token> tokens) -> parse_result;
}
