//! @file
//! @copyright See <a href="LICENSE.txt">LICENSE.txt</a>.

#pragma once

#include "tokens.hpp"
#include "visitation.hpp"

#include <memory>
#include <string>
#include <vector>

namespace gynjo::ast {
	//! Union type of all AST node types.
	using val =
		std::variant<struct assign, struct add, struct neg, struct sub, struct mul, struct div, struct exp, struct app, struct tup, tok::num, tok::sym>;

	//! Unique pointer to an AST node.
	using ptr = std::unique_ptr<val>;

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
			(elems.push_back(std::forward<Args>(args)), ...);
		}
	};

	//! Convenience function for creating an AST pointer from @p ast.
	template <typename T>
	auto make_ast(T&& ast) {
		return std::make_unique<ast::val>(std::forward<T>(ast));
	}

	//! Converts the AST pointer @p ast to a user-readable string.
	inline auto to_string(ptr const& ast) -> std::string {
		return match(
			*ast,
			[](assign const& assign) { return "(" + to_string(assign.symbol) + " = " + to_string(assign.rhs) + ")"; },
			[](add const& add) { return "(" + to_string(add.addend1) + " + " + to_string(add.addend2) + ")"; },
			[](neg const& neg) { return "(-" + to_string(neg.expr) + ")"; },
			[](sub const& sub) { return "(" + to_string(sub.minuend) + " - " + to_string(sub.subtrahend) + ")"; },
			[](mul const& mul) { return "(" + to_string(mul.factor1) + " * " + to_string(mul.factor2) + ")"; },
			[](div const& div) { return "(" + to_string(div.dividend) + " / " + to_string(div.divisor) + ")"; },
			[](exp const& exp) { return "(" + to_string(exp.base) + " ^ " + to_string(exp.exponent) + ")"; },
			[](app const& app) { return "(" + to_string(app.f) + "(" + to_string(app.arg) + "))"; },
			[](tup const& tup) {
				std::string result = "(";
				if (!tup.elems.empty()) {
					result += to_string(tup.elems.front());
					for (auto it = tup.elems.begin() + 1; it != tup.elems.end(); ++it) {
						result += ", " + to_string(*it);
					}
				}
				result += ")";
				return result;
			},
			[](tok::num const& num) { return num.rep; },
			[](tok::sym const& sym) { return sym.name; });
	}

	//! Creates a deep copy of @p ast.
	inline auto clone(ast::ptr const& ast) -> ast::ptr {
		return match(
			*ast,
			[](assign const& assign) {
				return make_ast(ast::assign{assign.symbol, clone(assign.rhs)});
			},
			[](add const& add) {
				return make_ast(ast::add{clone(add.addend1), clone(add.addend2)});
			},
			[](neg const& neg) { return make_ast(ast::neg{clone(neg.expr)}); },
			[](sub const& sub) {
				return make_ast(ast::sub{clone(sub.minuend), clone(sub.subtrahend)});
			},
			[](mul const& mul) {
				return make_ast(ast::mul{clone(mul.factor1), clone(mul.factor2)});
			},
			[](div const& div) {
				return make_ast(ast::div{clone(div.dividend), clone(div.divisor)});
			},
			[](exp const& exp) {
				return make_ast(ast::exp{clone(exp.base), clone(exp.exponent)});
			},
			[](app const& app) {
				return make_ast(ast::app{clone(app.f), clone(app.arg)});
			},
			[](tup const& tup) {
				ast::tup result;
				for (auto const& elem : tup.elems) {
					result.elems.push_back(clone(elem));
				}
				return make_ast(std::move(result));
			},
			[](tok::num const& num) { return make_ast(num); },
			[](tok::sym const& sym) { return make_ast(sym); });
	}
}
