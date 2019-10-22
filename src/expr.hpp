//! @file
//! @copyright See <a href="LICENSE.txt">LICENSE.txt</a>.

#pragma once

#include "expr_fwd.hpp"

#include "intrinsics.hpp"
#include "stmt_fwd.hpp"
#include "tokens.hpp"
#include "visitation.hpp"

#include <fmt/format.h>

#include <deque>
#include <memory>
#include <string>
#include <vector>

namespace gynjo {
	//! Conditional expression.
	struct cond {
		expr_ptr test;
		expr_ptr true_expr;
		expr_ptr false_expr;
		auto operator==(cond const&) const noexcept -> bool;
	};

	//! Block expression.
	struct block {
		std::unique_ptr<std::vector<stmt>> stmts;

		block();

		block(block const& that);
		block(block&&) noexcept = default;

		~block();

		auto operator=(block const& that) -> block&;
		auto operator=(block&&) noexcept -> block& = default;

		auto operator==(block const&) const noexcept -> bool;
	};

	//! Logical AND expression.
	struct and_ {
		expr_ptr left;
		expr_ptr right;
		auto operator==(and_ const&) const noexcept -> bool;
	};

	//! Logical OR expression.
	struct or_ {
		expr_ptr left;
		expr_ptr right;
		auto operator==(or_ const&) const noexcept -> bool;
	};

	//! Logical NOT expression.
	struct not_ {
		expr_ptr expr;
		auto operator==(not_ const&) const noexcept -> bool;
	};

	//! Less-than comparison expression.
	struct eq {
		expr_ptr left;
		expr_ptr right;
		auto operator==(eq const&) const noexcept -> bool;
	};

	//! Less-than comparison expression.
	struct neq {
		expr_ptr left;
		expr_ptr right;
		auto operator==(neq const&) const noexcept -> bool;
	};

	//! Less-than comparison expression.
	struct lt {
		expr_ptr left;
		expr_ptr right;
		auto operator==(lt const&) const noexcept -> bool;
	};

	//! Less-than-or-equal comparison expression.
	struct leq {
		expr_ptr left;
		expr_ptr right;
		auto operator==(leq const&) const noexcept -> bool;
	};

	//! Greater-than comparison expression.
	struct gt {
		expr_ptr left;
		expr_ptr right;
		auto operator==(gt const&) const noexcept -> bool;
	};

	//! Greater-than-or-equal comparison expression.
	struct geq {
		expr_ptr left;
		expr_ptr right;
		auto operator==(geq const&) const noexcept -> bool;
	};

	//! Addition expression.
	struct add {
		expr_ptr addend1;
		expr_ptr addend2;
		auto operator==(add const&) const noexcept -> bool;
	};

	//! Binary subtraction expression.
	struct sub {
		expr_ptr minuend;
		expr_ptr subtrahend;
		auto operator==(sub const&) const noexcept -> bool;
	};

	//! A cluster of function calls, exponentiations, (possibly implicit) multiplications, and/or divisions.
	//! @note This large grouping of operations is as fine-grained as possible in the parsing stage. Breaking this down
	//! into specific operations requires additional parsing in the evaluation stage since determining the order of
	//! operations requires type info.
	struct cluster {
		//! The way in which a cluster item is attached to the preceding elements of the cluster.
		enum class connector {
			adj_paren, // Adjacent value enclosed in parentheses
			adj_nonparen, // Adjacent value not enclosed in parentheses
			mul, // Explicit multiplication
			div, // Explicit division
			exp, // Explicit exponentiation
		};

		//! Determines whether the corresponding item is preceded by a negative sign.
		std::vector<bool> negations;

		//! A node in a function application, exponentiation, multiplication, or division.
		std::unique_ptr<std::vector<expr>> items;

		//! Connector i indicates how item i + 1 is connected to item i.
		std::vector<connector> connectors;

		cluster() = default;
		cluster(std::vector<bool> negations, std::unique_ptr<std::vector<expr>> items, std::vector<connector> connectors);

		cluster(cluster const& that);
		cluster(cluster&&) noexcept = default;

		auto operator=(cluster const& that) -> cluster&;
		auto operator=(cluster&&) noexcept -> cluster& = default;

		auto operator==(cluster const&) const noexcept -> bool;
	};

	//! Tuple expression.
	struct tup_expr {
		std::unique_ptr<std::vector<expr>> elems;

		tup_expr();
		explicit tup_expr(std::unique_ptr<std::vector<expr>> elems);

		tup_expr(tup_expr const& that);
		tup_expr(tup_expr&&) noexcept = default;

		auto operator=(tup_expr const& that) -> tup_expr&;
		auto operator=(tup_expr&&) noexcept -> tup_expr& = default;

		auto operator==(tup_expr const&) const noexcept -> bool;
	};

	template <typename... Args>
	auto make_tup_expr(Args&&... args) {
		auto elems = std::make_unique<std::vector<expr>>();
		(elems->push_back(std::move(args)), ...);
		return tup_expr{std::move(elems)};
	}

	//! List expression.
	struct list_expr {
		std::unique_ptr<std::deque<expr>> elems;

		list_expr();
		explicit list_expr(std::unique_ptr<std::deque<expr>> elems);

		list_expr(list_expr const& that);
		list_expr(list_expr&&) noexcept = default;

		auto operator=(list_expr const& that) -> list_expr&;
		auto operator=(list_expr&&) noexcept -> list_expr& = default;

		auto operator==(list_expr const&) const noexcept -> bool;
	};

	template <typename... Args>
	auto make_list_expr(Args&&... args) {
		auto elems = std::make_unique<std::vector<expr>>();
		(elems->push_back(std::move(args)), ...);
		return list_expr{std::move(elems)};
	}

	//! Lambda expression.
	struct lambda {
		expr_ptr params;

		//! The body of a lambda can be either a user-defined function or an intrinsic function.
		std::variant<expr_ptr, intrinsic> body;

		//! Only checks structural equality for the bodies (not functional equality) because of the halting problem.
		auto operator==(lambda const&) const noexcept -> bool;
	};

	//! Union type of all expression types.
	struct expr {
		std::variant<cond, block, and_, or_, not_, eq, neq, lt, leq, gt, geq, add, sub, cluster, lambda, tup_expr, list_expr, tok::boolean, tok::num, std::string, tok::sym> value;

		auto operator==(expr const&) const noexcept -> bool = default;
	};

	//! Convenience function for creating an expression pointer from @p expr.
	template <typename T>
	auto make_expr(T&& expr) {
		return std::make_shared<gynjo::expr>(gynjo::expr{std::forward<T>(expr)});
	}

	//! Converts @p expr to a user-readable string.
	auto to_string(expr const& expr) -> std::string;
}
