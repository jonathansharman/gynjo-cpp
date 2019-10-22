//! @file
//! @copyright See <a href="LICENSE.txt">LICENSE.txt</a>.

#pragma once

#include "intrinsics.hpp"
#include "visitation.hpp"

#include <compare>
#include <string>
#include <variant>

namespace gynjo::tok {
	//! Token for "import" keyword.
	struct imp {
		auto operator<=>(imp const&) const noexcept = default;
	};
	//! Token for "let" keyword.
	struct let {
		auto operator<=>(let const&) const noexcept = default;
	};
	//! Token for "if" keyword.
	struct if_ {
		auto operator<=>(if_ const&) const noexcept = default;
	};
	//! Token for "then" keyword.
	struct then {
		auto operator<=>(then const&) const noexcept = default;
	};
	//! Token for "else" keyword.
	struct else_ {
		auto operator<=>(else_ const&) const noexcept = default;
	};
	//! Token for "while" keyword.
	struct while_ {
		auto operator<=>(while_ const&) const noexcept = default;
	};
	//! Token for "for" keyword.
	struct for_ {
		auto operator<=>(for_ const&) const noexcept = default;
	};
	//! Token for "in" keyword.
	struct in {
		auto operator<=>(in const&) const noexcept = default;
	};
	//! Token for "do" keyword.
	struct do_ {
		auto operator<=>(do_ const&) const noexcept = default;
	};
	//! Token for "return" keyword.
	struct ret {
		auto operator<=>(ret const&) const noexcept = default;
	};
	//! Token for "and" keyword.
	struct and_ {
		auto operator<=>(and_ const&) const noexcept = default;
	};
	//! Token for "or" keyword.
	struct or_ {
		auto operator<=>(or_ const&) const noexcept = default;
	};
	//! Token for "not" keyword.
	struct not_ {
		auto operator<=>(not_ const&) const noexcept = default;
	};
	//! Token for "=".
	struct eq {
		auto operator<=>(eq const&) const noexcept = default;
	};
	//! Token for "!=".
	struct neq {
		auto operator<=>(neq const&) const noexcept = default;
	};
	//! Token for "<".
	struct lt {
		auto operator<=>(lt const&) const noexcept = default;
	};
	//! Token for "<=".
	struct leq {
		auto operator<=>(leq const&) const noexcept = default;
	};
	//! Token for ">".
	struct gt {
		auto operator<=>(gt const&) const noexcept = default;
	};
	//! Token for ">=".
	struct geq {
		auto operator<=>(geq const&) const noexcept = default;
	};
	//! Token for "+".
	struct plus {
		auto operator<=>(plus const&) const noexcept = default;
	};
	//! Token for "-".
	struct minus {
		auto operator<=>(minus const&) const noexcept = default;
	};
	//! Token for "*".
	struct mul {
		auto operator<=>(mul const&) const noexcept = default;
	};
	//! Token for "/".
	struct div {
		auto operator<=>(div const&) const noexcept = default;
	};
	//! Token for "^" or "**".
	struct exp {
		auto operator<=>(exp const&) const noexcept = default;
	};
	//! Token for "(".
	struct lparen {
		auto operator<=>(lparen const&) const noexcept = default;
	};
	//! Token for ")".
	struct rparen {
		auto operator<=>(rparen const&) const noexcept = default;
	};
	//! Token for "[".
	struct lsquare {
		auto operator<=>(lsquare const&) const noexcept = default;
	};
	//! Token for "]".
	struct rsquare {
		auto operator<=>(rsquare const&) const noexcept = default;
	};
	//! Token for "{".
	struct lcurly {
		auto operator<=>(lcurly const&) const noexcept = default;
	};
	//! Token for "}".
	struct rcurly {
		auto operator<=>(rcurly const&) const noexcept = default;
	};
	//! Token for ",".
	struct com {
		auto operator<=>(com const&) const noexcept = default;
	};
	//! Token for ";".
	struct semicolon {
		auto operator<=>(semicolon const&) const noexcept = default;
	};
	//! Token for "->".
	struct arrow {
		auto operator<=>(arrow const&) const noexcept = default;
	};
	//! Token for "?".
	struct que {
		auto operator<=>(que const&) const noexcept = default;
	};
	//! Token for ":".
	struct colon {
		auto operator<=>(colon const&) const noexcept = default;
	};
	struct boolean {
		bool value;
		auto operator<=>(boolean const&) const noexcept = default;
	};
	//! Token for floating-point numberical literals.
	struct num {
		std::string rep;
		auto operator<=>(num const&) const noexcept = default;
	};
	//! Token for symbols.
	struct sym {
		std::string name;
		auto operator<=>(sym const&) const noexcept = default;
	};

	//! Union type of all valid tokens.
	using token = std::variant<
		// clang-format off
		imp,
		let,
		if_, then, else_, // branch
		while_, for_, in, do_, ret, // loops/blocks
		and_, or_, not_, // boolean ops
		eq, neq, lt, leq, gt, geq, // comparison ops
		plus, minus, mul, div, exp, // arithmetic ops
		lparen, rparen, lsquare, rsquare, lcurly, rcurly, // brackets
		com, semicolon, arrow, que, colon, // punctuation
		boolean, num, sym, intrinsic, std::string // values
		// clang-format on
		>;

	//! Converts the token @p tok to a user-readable string.
	inline auto to_string(token tok) -> std::string {
		using namespace std::string_literals;
		return match(
			tok,
			[](imp const&) { return "import"s; },
			[](let const&) { return "let"s; },
			[](if_ const&) { return "if"s; },
			[](then const&) { return "then"s; },
			[](else_ const&) { return "else"s; },
			[](while_ const&) { return "while"s; },
			[](for_ const&) { return "for"s; },
			[](in const&) { return "in"s; },
			[](do_ const&) { return "do"s; },
			[](ret const&) { return "return"s; },
			[](and_ const&) { return "and"s; },
			[](or_ const&) { return "or"s; },
			[](not_ const&) { return "not"s; },
			[](eq const&) { return "="s; },
			[](neq const&) { return "!="s; },
			[](lt const&) { return "<"s; },
			[](leq const&) { return "<="s; },
			[](gt const&) { return ">"s; },
			[](geq const&) { return ">="s; },
			[](plus const&) { return "+"s; },
			[](minus const&) { return "-"s; },
			[](mul const&) { return "*"s; },
			[](div const&) { return "/"s; },
			[](exp const&) { return "^"s; },
			[](lparen const&) { return "("s; },
			[](rparen const&) { return ")"s; },
			[](lsquare const&) { return "["s; },
			[](rsquare const&) { return "]"s; },
			[](lcurly const&) { return "{"s; },
			[](rcurly const&) { return "}"s; },
			[](com const&) { return ","s; },
			[](semicolon const&) { return ";"s; },
			[](arrow const&) { return "->"s; },
			[](que const&) { return "?"s; },
			[](colon const&) { return ":"s; },
			[](boolean const& b) { return b.value ? "true"s : "false"s; },
			[](num const& n) { return n.rep; },
			[](sym const& s) { return s.name; },
			[](intrinsic const& f) { return name(f); },
			[](std::string const& str) { return '"' + str + '"'; });
	}
}
