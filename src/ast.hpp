//! @file
//! @copyright See <a href="LICENSE.txt">LICENSE.txt</a>.

#pragma once

#include "intrinsics.hpp"
#include "tokens.hpp"
#include "visitation.hpp"

#include <fmt/format.h>

#include <deque>
#include <memory>
#include <string>
#include <vector>

namespace gynjo::ast {
	//! Union type of all AST node types.
	using node = std::variant< //
		struct nop,
		struct imp,
		struct assign,
		struct cond,
		struct block,
		struct while_loop,
		struct for_loop,
		struct and_,
		struct or_,
		struct not_,
		struct eq,
		struct neq,
		struct lt,
		struct leq,
		struct gt,
		struct geq,
		struct add,
		struct sub,
		struct cluster,
		struct lambda,
		struct tup,
		struct list,
		tok::boolean,
		tok::num,
		tok::sym>;

	//! Shared pointer to an AST node.
	using ptr = std::shared_ptr<node>;

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
		ptr rhs;
		auto operator==(assign const&) const noexcept -> bool;
	};

	//! Conditional expression - if-then or if-then-else.
	struct cond {
		ptr test;
		ptr if_true;
		ptr if_false;
		auto operator==(cond const&) const noexcept -> bool;
	};

	//! Block expression.
	struct block {
		std::unique_ptr<std::vector<node>> stmts;

		block();

		block(block const& that);
		block(block&&) noexcept = default;

		auto operator=(block const& that) -> block&;
		auto operator=(block&&) noexcept -> block& = default;

		auto operator==(block const&) const noexcept -> bool;
	};

	//! While-loop expression.
	struct while_loop {
		ptr test;
		ptr body;
		auto operator==(while_loop const&) const noexcept -> bool;
	};

	//! For-loop expression.
	struct for_loop {
		tok::sym loop_var;
		ptr range;
		ptr body;
		auto operator==(for_loop const&) const noexcept -> bool;
	};

	//! Logical AND expression.
	struct and_ {
		ptr left;
		ptr right;
		auto operator==(and_ const&) const noexcept -> bool;
	};

	//! Logical OR expression.
	struct or_ {
		ptr left;
		ptr right;
		auto operator==(or_ const&) const noexcept -> bool;
	};

	//! Logical NOT expression.
	struct not_ {
		ptr expr;
		auto operator==(not_ const&) const noexcept -> bool;
	};

	//! Less-than comparison expression.
	struct eq {
		ptr left;
		ptr right;
		auto operator==(eq const&) const noexcept -> bool;
	};

	//! Less-than comparison expression.
	struct neq {
		ptr left;
		ptr right;
		auto operator==(neq const&) const noexcept -> bool;
	};

	//! Less-than comparison expression.
	struct lt {
		ptr left;
		ptr right;
		auto operator==(lt const&) const noexcept -> bool;
	};

	//! Less-than-or-equal comparison expression.
	struct leq {
		ptr left;
		ptr right;
		auto operator==(leq const&) const noexcept -> bool;
	};

	//! Greater-than comparison expression.
	struct gt {
		ptr left;
		ptr right;
		auto operator==(gt const&) const noexcept -> bool;
	};

	//! Greater-than-or-equal comparison expression.
	struct geq {
		ptr left;
		ptr right;
		auto operator==(geq const&) const noexcept -> bool;
	};

	//! Addition expression.
	struct add {
		ptr addend1;
		ptr addend2;
		auto operator==(add const&) const noexcept -> bool;
	};

	//! Binary subtraction expression.
	struct sub {
		ptr minuend;
		ptr subtrahend;
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
		std::unique_ptr<std::vector<node>> items;

		//! Connector i indicates how item i + 1 is connected to item i.
		std::vector<connector> connectors;

		cluster() = default;
		cluster(std::vector<bool> negations, std::unique_ptr<std::vector<node>> items, std::vector<connector> connectors);

		cluster(cluster const& that);
		cluster(cluster&&) noexcept = default;

		auto operator=(cluster const& that) -> cluster&;
		auto operator=(cluster&&) noexcept -> cluster& = default;

		auto operator==(cluster const&) const noexcept -> bool;
	};

	//! Tuple expression.
	struct tup {
		std::unique_ptr<std::vector<node>> elems;

		tup();
		explicit tup(std::unique_ptr<std::vector<node>> elems);

		tup(tup const& that);
		tup(tup&&) noexcept = default;

		auto operator=(tup const& that) -> tup&;
		auto operator=(tup&&) noexcept -> tup& = default;

		auto operator==(tup const&) const noexcept -> bool;
	};

	template <typename... Args>
	auto make_tup(Args&&... args) {
		auto elems = std::make_unique<std::vector<node>>();
		(elems->push_back(std::move(args)), ...);
		return tup{std::move(elems)};
	}

	//! List expression.
	struct list {
		std::unique_ptr<std::deque<node>> elems;

		list();
		explicit list(std::unique_ptr<std::deque<node>> elems);

		list(list const& that);
		list(list&&) noexcept = default;

		auto operator=(list const& that) -> list&;
		auto operator=(list&&) noexcept -> list& = default;

		auto operator==(list const&) const noexcept -> bool;
	};

	template <typename... Args>
	auto make_list(Args&&... args) {
		auto elems = std::make_unique<std::vector<node>>();
		(elems->push_back(std::move(args)), ...);
		return list{std::move(elems)};
	}

	//! Lambda expression.
	struct lambda {
		ptr params;

		//! The body of a lambda can be either a user-defined function or an intrinsic function.
		std::variant<ptr, intrinsic> body;

		//! Only checks structural equality for the bodies (not functional equality) because of the halting problem.
		auto operator==(lambda const&) const noexcept -> bool;
	};

	//! Convenience function for creating an AST node pointer from @p node.
	template <typename T>
	auto make_node(T&& node) {
		return std::make_shared<ast::node>(std::forward<T>(node));
	}

	//! Converts the AST node @p node to a user-readable string.
	auto to_string(node const& node) -> std::string;
}
