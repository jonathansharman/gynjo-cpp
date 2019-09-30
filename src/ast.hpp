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
	using node =
		std::variant<struct assign, struct add, struct neg, struct sub, struct cluster, struct lambda, struct tup, tok::num, tok::sym>;

	//! Unique pointer to an AST node.
	using ptr = std::unique_ptr<node>;

	//! Assignment expression.
	struct assign {
		tok::sym symbol;
		ptr rhs;
	};
	//! Addition expression.
	struct add {
		ptr addend1;
		ptr addend2;
	};
	//! Negation expression.
	struct neg {
		ptr expr;
	};
	//! Binary subtraction expression.
	struct sub {
		ptr minuend;
		ptr subtrahend;
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

		std::vector<ptr> items;
		//! Connector i says how item i + 1 is connected to item i.
		std::vector<connector> connectors;
	};
	//! Tuple expression.
	struct tup {
		std::vector<ptr> elems;

		template <typename... Args>
		tup(Args&&... args) {
			(elems.push_back(std::move(args)), ...);
		}
	};
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

	//! Creates a deep copy of @p ast.
	auto clone(ast::node const& node) -> ast::ptr;
}
