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
		std::variant<struct assign, struct add, struct neg, struct sub, struct cluster, struct fun, struct tup, tok::num, tok::sym>;

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

		ptr first;
		std::vector<std::pair<connector, ptr>> rest;
	};
	// Tuple expression.
	struct tup {
		std::vector<ptr> elems;

		template <typename... Args>
		tup(Args&&... args) {
			(elems.push_back(std::move(args)), ...);
		}
	};
	// Lambda function expression.
	struct fun {
		ptr params;
		ptr body;
	};

	//! Convenience function for creating an AST node pointer from @p node.
	template <typename T>
	auto make_node(T&& node) {
		return std::make_unique<ast::node>(std::forward<T>(node));
	}

	//! Converts the AST node @p node to a user-readable string.
	inline auto to_string(node const& node) -> std::string {
		return match(
			node,
			[](assign const& assign) {
				return "(" + ast::to_string(assign.symbol) + " = " + to_string(*assign.rhs) + ")";
			},
			[](add const& add) { return fmt::format("({} + {})", to_string(*add.addend1), to_string(*add.addend2)); },
			[](neg const& neg) { return fmt::format("(-{})", to_string(*neg.expr)); },
			[](sub const& sub) { return fmt::format("({} - {})", to_string(*sub.minuend), to_string(*sub.subtrahend)); },
			[](cluster const& cluster) {
				std::string result = "(" + to_string(*cluster.first);
				for (auto const& item : cluster.rest) {
					switch (item.first) {
						case cluster::connector::adj_paren:
							result += " (" + to_string(*item.second) + ")";
							break;
						case cluster::connector::adj_nonparen:
							result += " " + to_string(*item.second);
							break;
						case cluster::connector::mul:
							result += " * " + to_string(*item.second);
							break;
						case cluster::connector::div:
							result += " / " + to_string(*item.second);
							break;
						case cluster::connector::exp:
							result += " ^ " + to_string(*item.second);
							break;
					}
				}
				result += ")";
				return result;
			},
			[](fun const& f) { return fmt::format("({} -> {})", to_string(*f.params), to_string(*f.body)); },
			[](tup const& tup) {
				std::string result = "(";
				if (!tup.elems.empty()) {
					result += to_string(*tup.elems.front());
					for (auto it = tup.elems.begin() + 1; it != tup.elems.end(); ++it) {
						result += ", " + to_string(**it);
					}
				}
				result += ")";
				return result;
			},
			[](tok::num const& num) { return num.rep; },
			[](tok::sym const& sym) { return sym.name; });
	}

	//! Creates a deep copy of @p ast.
	inline auto clone(ast::node const& node) -> ast::ptr {
		return match(
			node,
			[](assign const& assign) {
				return make_node(ast::assign{assign.symbol, clone(*assign.rhs)});
			},
			[](add const& add) {
				return make_node(ast::add{clone(*add.addend1), clone(*add.addend2)});
			},
			[](neg const& neg) { return make_node(ast::neg{clone(*neg.expr)}); },
			[](sub const& sub) {
				return make_node(ast::sub{clone(*sub.minuend), clone(*sub.subtrahend)});
			},
			[](cluster const& c) {
				std::vector<std::pair<cluster::connector, ptr>> rest;
				for (auto const& item : c.rest) {
					rest.emplace_back(item.first, clone(*item.second));
				}
				return make_node(cluster{clone(*c.first), std::move(rest)});
			},
			[](fun const& f) {
				return make_node(ast::fun{clone(*f.params), clone(*f.body)});
			},
			[](tup const& tup) {
				ast::tup result;
				for (auto const& elem : tup.elems) {
					result.elems.push_back(clone(*elem));
				}
				return make_node(std::move(result));
			},
			[](tok::num const& num) { return make_node(num); },
			[](tok::sym const& sym) { return make_node(sym); });
	}
}
