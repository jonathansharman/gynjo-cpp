//! @file
//! @copyright See <a href="LICENSE.txt">LICENSE.txt</a>.

#pragma once

#include "visitation.hpp"

#include <compare>
#include <string>
#include <variant>

namespace gynjo::tok {
	struct eq {
		auto operator<=>(eq const&) const = default;
	};
	struct plus {
		auto operator<=>(plus const&) const = default;
	};
	struct minus {
		auto operator<=>(minus const&) const = default;
	};
	struct mul {
		auto operator<=>(mul const&) const = default;
	};
	struct div {
		auto operator<=>(div const&) const = default;
	};
	struct exp {
		auto operator<=>(exp const&) const = default;
	};
	struct lft {
		auto operator<=>(lft const&) const = default;
	};
	struct rht {
		auto operator<=>(rht const&) const = default;
	};
	struct num {
		std::string rep;
		auto operator<=>(num const&) const = default;
	};
	struct sym {
		std::string name;
		auto operator<=>(sym const&) const = default;
	};

	using token = std::variant<eq, plus, minus, mul, div, exp, lft, rht, num, sym>;

	inline auto to_string(token token) -> std::string {
		using namespace std::string_literals;
		return match(
			token,
			[&](eq const&) { return "="s; },
			[&](plus const&) { return "+"s; },
			[&](minus const&) { return "-"s; },
			[&](mul const&) { return "*"s; },
			[&](div const&) { return "/"s; },
			[&](exp const&) { return "^"s; },
			[&](lft const&) { return "("s; },
			[&](rht const&) { return ")"s; },
			[&](num const& n) { return n.rep; },
			[&](sym const& s) { return s.name; });
	}
}
