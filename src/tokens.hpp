//! @file
//! @copyright See <a href="LICENSE.txt">LICENSE.txt</a>.

#pragma once

#include "visitation.hpp"

#include <compare>
#include <string>
#include <variant>

namespace gynjo::tok {
	//! Token for "import" keyword.
	struct imp {
		auto operator<=>(imp const&) const = default;
	};
	//! Token for "if" keyword.
	struct if_ {
		auto operator<=>(if_ const&) const = default;
	};
	//! Token for "then" keyword.
	struct then {
		auto operator<=>(then const&) const = default;
	};
	//! Token for "else" keyword.
	struct else_ {
		auto operator<=>(else_ const&) const = default;
	};
	//! Token for "=".
	struct eq {
		auto operator<=>(eq const&) const = default;
	};
	//! Token for "+".
	struct plus {
		auto operator<=>(plus const&) const = default;
	};
	//! Token for "-".
	struct minus {
		auto operator<=>(minus const&) const = default;
	};
	//! Token for "*".
	struct mul {
		auto operator<=>(mul const&) const = default;
	};
	//! Token for "/".
	struct div {
		auto operator<=>(div const&) const = default;
	};
	//! Token for "^" or "**".
	struct exp {
		auto operator<=>(exp const&) const = default;
	};
	//! Token for "(".
	struct lft {
		auto operator<=>(lft const&) const = default;
	};
	//! Token for ")".
	struct rht {
		auto operator<=>(rht const&) const = default;
	};
	//! Token for ",".
	struct com {
		auto operator<=>(com const&) const = default;
	};
	//! Token for "->".
	struct arrow {
		auto operator<=>(arrow const&) const = default;
	};
	struct boolean {
		bool value;
		auto operator<=>(boolean const&) const = default;
	};
	//! Token for floating-point numberical literals.
	struct num {
		std::string rep;
		auto operator<=>(num const&) const = default;
	};
	//! Token for symbols.
	struct sym {
		std::string name;
		auto operator<=>(sym const&) const = default;
	};

	//! Union type of all valid tokens.
	using token = std::variant<imp, if_, then, else_, eq, plus, minus, mul, div, exp, lft, rht, com, arrow, boolean, num, sym>;

	//! Converts the token @p tok to a user-readable string.
	inline auto to_string(token tok) -> std::string {
		using namespace std::string_literals;
		return match(
			tok,
			[](imp const&) { return "import"s; },
			[](if_ const&) { return "if"s; },
			[](then const&) { return "then"s; },
			[](else_ const&) { return "else"s; },
			[](eq const&) { return "="s; },
			[](plus const&) { return "+"s; },
			[](minus const&) { return "-"s; },
			[](mul const&) { return "*"s; },
			[](div const&) { return "/"s; },
			[](exp const&) { return "^"s; },
			[](lft const&) { return "("s; },
			[](rht const&) { return ")"s; },
			[](com const&) { return ","s; },
			[](arrow const&) { return "->"s; },
			[](boolean const& b) { return b.value ? "true"s : "false"s; },
			[](num const& n) { return n.rep; },
			[](sym const& s) { return s.name; });
	}
}
