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
		bool operator==(nop const&) const = default;
	};

	//! Import statement.
	struct imp {
		std::string filename;

		bool operator==(imp const&) const = default;
	};

	//! Assignment statement.
	struct assign {
		tok::sym symbol;
		ptr rhs;

		explicit assign(tok::sym symbol, ptr rhs);

		assign(assign const& that);
		assign(assign&&) = default;

		assign& operator=(assign const& that);
		assign& operator=(assign&&) = default;

		bool operator==(assign const& that) const;
	};

	//! Addition expression.
	struct add {
		ptr addend1;
		ptr addend2;

		add(ptr addend1, ptr addend2);

		add(add const& that);
		add(add&&) = default;

		add& operator=(add const& that);
		add& operator=(add&&) = default;

		bool operator==(add const& that) const;
	};

	//! Binary subtraction expression.
	struct sub {
		ptr minuend;
		ptr subtrahend;

		sub(ptr minuend, ptr subtrahend);

		sub(sub const& that);
		sub(sub&&) = default;

		sub& operator=(sub const& that);
		sub& operator=(sub&&) = default;

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
		cluster(cluster&&) = default;

		cluster& operator=(cluster const& that);
		cluster& operator=(cluster&&) = default;

		bool operator==(cluster const& that) const;
	};

	//! Tuple expression.
	struct tup {
		std::unique_ptr<std::vector<node>> elems;

		tup();
		explicit tup(std::unique_ptr<std::vector<node>> elems);

		tup(tup const& that);
		tup(tup&&) = default;

		tup& operator=(tup const& that);
		tup& operator=(tup&&) = default;

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
		lambda(lambda&&) = default;

		lambda& operator=(lambda const& that);
		lambda& operator=(lambda&&) = default;

		//! Just structural equality because of the halting problem.
		bool operator==(lambda const& that) const = default;
	};

	//! Convenience function for creating an AST node pointer from @p node.
	template <typename T>
	auto make_node(T&& node) {
		return std::make_unique<ast::node>(std::forward<T>(node));
	}

	//! Converts the AST node @p node to a user-readable string.
	auto to_string(node const& node) -> std::string;
}
