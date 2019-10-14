//! @file
//! @copyright See <a href="LICENSE.txt">LICENSE.txt</a>.

#pragma once

#include "stmt_fwd.hpp"

#include "expr_fwd.hpp"
#include "intrinsics.hpp"
#include "tokens.hpp"
#include "visitation.hpp"

#include <fmt/format.h>

#include <deque>
#include <memory>
#include <string>
#include <vector>

namespace gynjo {
	//! No-op - statement that does nothing.
	struct nop {
		auto operator==(nop const&) const noexcept -> bool = default;
	};

	//! Import statement.
	struct imp {
		std::string filename;
		auto operator==(imp const&) const noexcept -> bool = default;
	};

	//! Assignment statement.
	struct assign {
		tok::sym symbol;
		expr_ptr rhs;
		auto operator==(assign const&) const noexcept -> bool;
	};

	//! Conditional branch - if-then or if-then-else.
	struct branch {
		expr_ptr test;
		stmt_ptr true_stmt;
		stmt_ptr false_stmt;
		auto operator==(branch const&) const noexcept -> bool;
	};

	//! While-loop statement.
	struct while_loop {
		expr_ptr test;
		stmt_ptr body;
		auto operator==(while_loop const&) const noexcept -> bool;
	};

	//! For-loop statement.
	struct for_loop {
		tok::sym loop_var;
		expr_ptr range;
		stmt_ptr body;
		auto operator==(for_loop const&) const noexcept -> bool;
	};

	//! Return statement.
	struct ret {
		expr_ptr result;
		auto operator==(ret const&) const noexcept -> bool;
	};

	//! An expression used as a statement, e.g. "print(1);".
	struct expr_stmt {
		expr_ptr expr;
		auto operator==(expr_stmt const&) const noexcept -> bool;
	};

	//! Union type of all statment types.
	struct stmt {
		std::variant<nop, imp, assign, branch, while_loop, for_loop, ret, expr_stmt> value;

		auto operator==(stmt const&) const noexcept -> bool = default;
	};

	//! Convenience function for creating a statement pointer from @p stmt.
	template <typename T>
	auto make_stmt(T&& stmt) {
		return std::make_shared<gynjo::stmt>(gynjo::stmt{std::forward<T>(stmt)});
	}

	//! Converts @p stmt to a user-readable string.
	auto to_string(stmt const& stmt) -> std::string;
}
