//! @file
//! @copyright See <a href="LICENSE.txt">LICENSE.txt</a>.

#pragma once

#include "visitation.hpp"

#include <compare>
#include <string>
#include <variant>

namespace gynjo::tok {
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
	using token = std::variant<eq, plus, minus, mul, div, exp, lft, rht, com, num, sym>;

	//! Converts the token @p tok to a user-readable string.
	inline auto to_string(token tok) -> std::string {
		using namespace std::string_literals;
		return match(
			tok,
			[&](eq const&) { return "="s; },
			[&](plus const&) { return "+"s; },
			[&](minus const&) { return "-"s; },
			[&](mul const&) { return "*"s; },
			[&](div const&) { return "/"s; },
			[&](exp const&) { return "^"s; },
			[&](lft const&) { return "("s; },
			[&](rht const&) { return ")"s; },
			[&](com const&) { return ","s; },
			[&](num const& n) { return n.rep; },
			[&](sym const& s) { return s.name; });
	}
}
