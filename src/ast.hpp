//! @file
//! @copyright See <a href="LICENSE.txt">LICENSE.txt</a>.

#pragma once

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

	//! Unique pointer to an AST node.
	using ptr = std::unique_ptr<node>;

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

		explicit assign(tok::sym symbol, ptr rhs);

		assign(assign const& that);
		assign(assign&&) noexcept = default;

		assign& operator=(assign const& that);
		assign& operator=(assign&&) noexcept = default;

		bool operator==(assign const& that) const;
	};

	//! Logical AND expression.
	struct and_ {
		ptr left;
		ptr right;

		and_(ptr left, ptr right);

		and_(and_ const& that);
		and_(and_&&) noexcept = default;

		and_& operator=(and_ const& that);
		and_& operator=(and_&&) noexcept = default;

		bool operator==(and_ const& that) const;
	};

	//! Logical OR expression.
	struct or_ {
		ptr left;
		ptr right;

		or_(ptr left, ptr right);

		or_(or_ const& that);
		or_(or_&&) noexcept = default;

		or_& operator=(or_ const& that);
		or_& operator=(or_&&) noexcept = default;

		bool operator==(or_ const& that) const;
	};

	//! Logical NOT expression.
	struct not_ {
		ptr expr;

		not_(ptr expr);

		not_(not_ const& that);
		not_(not_&&) noexcept = default;

		not_& operator=(not_ const& that);
		not_& operator=(not_&&) noexcept = default;

		bool operator==(not_ const& that) const;
	};

	//! Less-than comparison expression.
	struct eq {
		ptr left;
		ptr right;

		eq(ptr left, ptr right);

		eq(eq const& that);
		eq(eq&&) noexcept = default;

		eq& operator=(eq const& that);
		eq& operator=(eq&&) noexcept = default;

		bool operator==(eq const& that) const;
	};

	//! Less-than comparison expression.
	struct neq {
		ptr left;
		ptr right;

		neq(ptr left, ptr right);

		neq(neq const& that);
		neq(neq&&) noexcept = default;

		neq& operator=(neq const& that);
		neq& operator=(neq&&) noexcept = default;

		bool operator==(neq const& that) const;
	};

	//! Less-than comparison expression.
	struct lt {
		ptr left;
		ptr right;

		lt(ptr left, ptr right);

		lt(lt const& that);
		lt(lt&&) noexcept = default;

		lt& operator=(lt const& that);
		lt& operator=(lt&&) noexcept = default;

		bool operator==(lt const& that) const;
	};

	//! Less-than-or-equal comparison expression.
	struct leq {
		ptr left;
		ptr right;

		leq(ptr left, ptr right);

		leq(leq const& that);
		leq(leq&&) noexcept = default;

		leq& operator=(leq const& that);
		leq& operator=(leq&&) noexcept = default;

		bool operator==(leq const& that) const;
	};

	//! Greater-than comparison expression.
	struct gt {
		ptr left;
		ptr right;

		gt(ptr left, ptr right);

		gt(gt const& that);
		gt(gt&&) noexcept = default;

		gt& operator=(gt const& that);
		gt& operator=(gt&&) noexcept = default;

		bool operator==(gt const& that) const;
	};

	//! Greater-than-or-equal comparison expression.
	struct geq {
		ptr left;
		ptr right;

		geq(ptr left, ptr right);

		geq(geq const& that);
		geq(geq&&) noexcept = default;

		geq& operator=(geq const& that);
		geq& operator=(geq&&) noexcept = default;

		bool operator==(geq const& that) const;
	};

	//! Addition expression.
	struct add {
		ptr addend1;
		ptr addend2;

		add(ptr addend1, ptr addend2);

		add(add const& that);
		add(add&&) noexcept = default;

		add& operator=(add const& that);
		add& operator=(add&&) noexcept = default;

		bool operator==(add const& that) const;
	};

	//! Binary subtraction expression.
	struct sub {
		ptr minuend;
		ptr subtrahend;

		sub(ptr minuend, ptr subtrahend);

		sub(sub const& that);
		sub(sub&&) noexcept = default;

		sub& operator=(sub const& that);
		sub& operator=(sub&&) noexcept = default;

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
		ptr body;

		lambda(ptr params, ptr body);

		lambda(lambda const& that);
		lambda(lambda&&) noexcept = default;

		lambda& operator=(lambda const& that);
		lambda& operator=(lambda&&) noexcept = default;

		//! Just structural equality because of the halting problem.
		bool operator==(lambda const& that) const noexcept = default;
	};

	//! Convenience function for creating an AST node pointer from @p node.
	template <typename T>
	auto make_node(T&& node) {
		return std::make_unique<ast::node>(std::forward<T>(node));
	}

	//! Converts the AST node @p node to a user-readable string.
	auto to_string(node const& node) -> std::string;
}
