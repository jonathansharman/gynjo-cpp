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
	//! Constant iterator into a vector of tokens.
	using token_it = std::vector<tok::token>::const_iterator;

	//! An iterator to the next unused token along with a parsed AST node.
	struct it_node {
		token_it it;
		ast::node node;
	};

	//! Either a (token iterator, AST node) pair or an error message.
	using parse_result = tl::expected<it_node, std::string>;

	//! If possible, parses the next single expression from @p begin to @p end.
	//! @return An iterator to the next unused token along with the parsed AST node, or an error message.
	auto parse_expr(token_it begin, token_it end) -> parse_result;

	//! If possible, parses the next single statement from @p begin to @p end.
	//! @return An iterator to the next unused token along with the parsed AST node, or an error message.
	auto parse_stmt(token_it begin, token_it end) -> parse_result;
}
