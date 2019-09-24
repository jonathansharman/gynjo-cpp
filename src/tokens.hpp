//! @file
//! @copyright See <a href="LICENSE.txt">LICENSE.txt</a>.

#pragma once

#include "visitation.hpp"

#include <compare>
#include <string>
#include <variant>

namespace gynjo::tok {
	struct add {
		auto operator<=>(add const&) const = default;
	};
	struct sub {
		auto operator<=>(sub const&) const = default;
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
		double value;
		auto operator<=>(num const&) const = default;
	};
	struct sym {
		std::string name;
		auto operator<=>(sym const&) const = default;
	};

	using token = std::variant<add, sub, mul, div, exp, lft, rht, num, sym>;

	inline auto to_string(token token) -> std::string {
		using namespace std::string_literals;
		return match(
			token,
			[&](add const&) { return "+"s; },
			[&](sub const&) { return "-"s; },
			[&](mul const&) { return "*"s; },
			[&](div const&) { return "/"s; },
			[&](exp const&) { return "^"s; },
			[&](lft const&) { return "("s; },
			[&](rht const&) { return ")"s; },
			[&](num const& n) { return std::to_string(n.value); },
			[&](sym const& s) { return s.name; });
	}
}
