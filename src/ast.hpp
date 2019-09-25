//! @file
//! @copyright See <a href="LICENSE.txt">LICENSE.txt</a>.

#pragma once

#include "tokens.hpp"
#include "visitation.hpp"

#include <memory>
#include <string>
#include <variant>

namespace gynjo::ast {
	using val = std::variant<struct assign, struct add, struct sub, struct mul, struct div, struct exp, struct num, tok::sym>;
	using ptr = std::unique_ptr<val>;

	struct assign {
		tok::sym symbol;
		ptr rhs;
	};
	struct add {
		ptr a;
		ptr b;
	};
	struct sub {
		ptr a;
		ptr b;
	};
	struct mul {
		ptr a;
		ptr b;
	};
	struct div {
		ptr a;
		ptr b;
	};
	struct exp {
		ptr a;
		ptr b;
	};
	struct num {
		double value;
	};

	template <typename T>
	auto make_ast(T&& val) {
		return std::make_unique<ast::val>(std::forward<T>(val));
	}

	inline auto to_string(ptr const& ast) -> std::string {
		return match(
			*ast,
			[](assign const& assign) { return "(" + to_string(assign.symbol) + " = " + to_string(assign.rhs) + ")"; },
			[](add const& add) { return "(" + to_string(add.a) + " + " + to_string(add.b) + ")"; },
			[](sub const& sub) { return "(" + to_string(sub.a) + " - " + to_string(sub.b) + ")"; },
			[](mul const& mul) { return "(" + to_string(mul.a) + " * " + to_string(mul.b) + ")"; },
			[](div const& div) { return "(" + to_string(div.a) + " / " + to_string(div.b) + ")"; },
			[](exp const& exp) { return "(" + to_string(exp.a) + " ^ " + to_string(exp.b) + ")"; },
			[](num const& num) { return std::to_string(num.value); },
			[](tok::sym const& sym) { return sym.name; });
	}
}
