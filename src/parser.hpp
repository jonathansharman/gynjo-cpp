//! @file
//! @copyright See <a href="LICENSE.txt">LICENSE.txt</a>.

#pragma once

#include "ast.hpp"
#include "tokens.hpp"
#include "values.hpp"

#include <tl/expected.hpp>

#include <string>
#include <unordered_map>

namespace gynjo {
	//! Either an AST pointer or an error message.
	using parse_result = tl::expected<ast::ptr, std::string>;

	//! A map from symbol names to their Gynjo values.
	using environment = std::unordered_map<std::string, val::value>;

	//! Parses @p tokens into an AST in the context of @p env, if possible.
	auto parse(environment& env, std::vector<tok::token> tokens) -> parse_result;
}
