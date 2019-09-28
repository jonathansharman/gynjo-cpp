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
		std::variant<struct assign, struct add, struct neg, struct sub, struct mul, struct div, struct exp, struct app, struct fun, struct tup, tok::num, tok::sym>;

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
	//! Multiplication expression.
	struct mul {
		ptr factor1;
		ptr factor2;
	};
	//! Division expression.
	struct div {
		ptr dividend;
		ptr divisor;
	};
	//! Exponentiation expression.
	struct exp {
		ptr base;
		ptr exponent;
	};
	// Function application expression.
	struct app {
		ptr f;
		ptr arg;
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
			[](mul const& mul) { return fmt::format("({} * {})", to_string(*mul.factor1), to_string(*mul.factor2)); },
			[](div const& div) { return fmt::format("({} / {})", to_string(*div.dividend), to_string(*div.divisor)); },
			[](exp const& exp) { return fmt::format("({} ^ {})", to_string(*exp.base), to_string(*exp.exponent)); },
			[](app const& app) { return fmt::format("({} {})", to_string(*app.f), to_string(*app.arg)); },
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
			[](mul const& mul) {
				return make_node(ast::mul{clone(*mul.factor1), clone(*mul.factor2)});
			},
			[](div const& div) {
				return make_node(ast::div{clone(*div.dividend), clone(*div.divisor)});
			},
			[](exp const& exp) {
				return make_node(ast::exp{clone(*exp.base), clone(*exp.exponent)});
			},
			[](app const& app) {
				return make_node(ast::app{clone(*app.f), clone(*app.arg)});
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
