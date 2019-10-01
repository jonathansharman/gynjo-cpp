//! @file
//! @copyright See <a href="LICENSE.txt">LICENSE.txt</a>.

#include "lexer.hpp"

#ifndef _DEBUG
#define DOCTEST_CONFIG_DISABLE
#endif
#include <doctest/doctest.h>

TEST_SUITE("lexer") {
	using namespace gynjo;
	using namespace gynjo::tok;
	// Resolve collisions.
	using tok::div;
	using tok::exp;

	TEST_CASE("whitespace") {
		auto const expected = std::vector<token>{num{"1"}, plus{}, num{"2"}, plus{}, num{"3"}};
		auto const actual = lex(" \t \n 1 \n + \t 2+3 \t \n ");
		CHECK(expected == actual.value());
	}

	TEST_CASE("numbers, operators, and separators") {
		auto const expected = std::vector<token>{//
			num{"5"},
			mul{},
			lft{},
			num{"1"},
			plus{},
			minus{},
			arrow{},
			num{"2"},
			rht{},
			exp{},
			exp{},
			mul{},
			eq{},
			div{},
			num{".1"},
			num{"0"},
			num{"0.1"},
			com{}};
		auto const actual = lex("5*(1+-->2)^***=/.1 0 0.1,");
		CHECK(expected == actual.value());
	}

	TEST_CASE("line comments") {
		auto const expected = std::vector<token>{num{"1"}, plus{}, num{"2"}};
		auto const actual = lex("1+2 // This is a line comment.");
		CHECK(expected == actual.value());
	}

	TEST_CASE("key words") {
		auto const expected = std::vector<token>{//
			sym{"imports"},
			imp{},
			sym{"ifs"},
			if_{},
			sym{"thens"},
			then{},
			sym{"elses"},
			else_{}};
		auto const actual = lex("imports import ifs if thens then elses else");
		CHECK(expected == actual.value());
	}
}
