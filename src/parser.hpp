//! @file
//! @copyright See <a href="LICENSE.txt">LICENSE.txt</a>.

#pragma once

#include "expr.hpp"
#include "stmt.hpp"
#include "tokens.hpp"
#include "values.hpp"

#include <tl/expected.hpp>

#include <string>
#include <vector>

namespace gynjo {
	//! Constant iterator into a vector of tokens.
	using token_it = std::vector<tok::token>::const_iterator;

	//! An iterator to the next unused token along with a parsed expression.
	struct it_expr {
		token_it it;
		expr expr;
	};

	//! An iterator to the next unused token along with a parsed statement.
	struct it_stmt {
		token_it it;
		stmt stmt;
	};

	//! Either a (token iterator, expression) pair or an error message.
	using parse_expr_result = tl::expected<it_expr, std::string>;

	//! Either a (token iterator, statement) pair or an error message.
	using parse_stmt_result = tl::expected<it_stmt, std::string>;

	//! If possible, parses the next single expression from @p begin to @p end.
	//! @return An iterator to the next unused token along with the parsed expression, or an error message.
	auto parse_expr(token_it begin, token_it end) -> parse_expr_result;

	//! If possible, parses the next single statement from @p begin to @p end.
	//! @return An iterator to the next unused token along with the parsed statement, or an error message.
	auto parse_stmt(token_it begin, token_it end) -> parse_stmt_result;
}
