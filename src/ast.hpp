//! @file
//! @copyright See <a href="LICENSE.txt">LICENSE.txt</a>.

#pragma once

#include "intrinsics.hpp"
#include "tokens.hpp"
#include "visitation.hpp"

#include <fmt/format.h>

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
		tok::boolean,
		tok::num,
		tok::sym>;

	//! Shared pointer to an AST node.
	using ptr = std::shared_ptr<node>;

	//! No-op - statement that does nothing.
	struct nop {
		bool operator==(nop const&) const noexcept = default;
	};

	//! Import statement.
	struct imp {
		std::string filename;

		bool operator==(imp const&) const noexcept = default;
	};

	//! Assignment statement.
	struct assign {
		tok::sym symbol;
		ptr rhs;

		bool operator==(assign const& that) const;
	};

	//! Conditional expression - if-then or if-then-else.
	struct cond {
		ptr test;
		ptr if_true;
		ptr if_false;

		bool operator==(cond const& that) const;
	};

	//! Logical AND expression.
	struct and_ {
		ptr left;
		ptr right;

		bool operator==(and_ const& that) const;
	};

	//! Logical OR expression.
	struct or_ {
		ptr left;
		ptr right;

		bool operator==(or_ const& that) const;
	};

	//! Logical NOT expression.
	struct not_ {
		ptr expr;

		bool operator==(not_ const& that) const;
	};

	//! Less-than comparison expression.
	struct eq {
		ptr left;
		ptr right;

		bool operator==(eq const& that) const;
	};

	//! Less-than comparison expression.
	struct neq {
		ptr left;
		ptr right;

		bool operator==(neq const& that) const;
	};

	//! Less-than comparison expression.
	struct lt {
		ptr left;
		ptr right;

		bool operator==(lt const& that) const;
	};

	//! Less-than-or-equal comparison expression.
	struct leq {
		ptr left;
		ptr right;

		bool operator==(leq const& that) const;
	};

	//! Greater-than comparison expression.
	struct gt {
		ptr left;
		ptr right;

		bool operator==(gt const& that) const;
	};

	//! Greater-than-or-equal comparison expression.
	struct geq {
		ptr left;
		ptr right;

		bool operator==(geq const& that) const;
	};

	//! Addition expression.
	struct add {
		ptr addend1;
		ptr addend2;

		bool operator==(add const& that) const;
	};

	//! Binary subtraction expression.
	struct sub {
		ptr minuend;
		ptr subtrahend;

		bool operator==(sub const& that) const;
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

		cluster& operator=(cluster const& that);
		cluster& operator=(cluster&&) noexcept = default;

		bool operator==(cluster const& that) const;
	};

	//! Tuple expression.
	struct tup {
		std::unique_ptr<std::vector<node>> elems;

		tup();
		explicit tup(std::unique_ptr<std::vector<node>> elems);

		tup(tup const& that);
		tup(tup&&) noexcept = default;

		tup& operator=(tup const& that);
		tup& operator=(tup&&) noexcept = default;

		bool operator==(tup const& that) const;
	};

	template <typename... Args>
	auto make_tup(Args&&... args) {
		auto elems = std::make_unique<std::vector<node>>();
		(elems->push_back(std::move(args)), ...);
		return tup{std::move(elems)};
	}

	//! Lambda expression.
	struct lambda {
		ptr params;
		//! The body of a lambda can be either a user-defined function or an intrinsic function.
		std::variant<ptr, intrinsic> body;

		//! Only checks structural equality for the bodies (not functional equality) because of the halting problem.
		bool operator==(lambda const& that) const noexcept;
	};

	//! Convenience function for creating an AST node pointer from @p node.
	template <typename T>
	auto make_node(T&& node) {
		return std::make_shared<ast::node>(std::forward<T>(node));
	}

	//! Converts the AST node @p node to a user-readable string.
	auto to_string(node const& node) -> std::string;
}
