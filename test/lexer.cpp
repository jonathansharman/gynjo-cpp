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
			let{},
			eq{},
			neq{},
			lt{},
			leq{},
			gt{},
			geq{},
			mul{},
			lparen{},
			plus{},
			minus{},
			arrow{},
			rparen{},
			lsquare{},
			rsquare{},
			exp{},
			exp{},
			mul{},
			div{},
			num{".1"},
			num{"0"},
			num{"0.1"},
			com{},
			que{},
			colon{}};
		auto const actual = lex("let=!=<<=>>=*(+-->)[]^***/.1 0 0.1,?:");
		CHECK(expected == actual.value());
	}

	TEST_CASE("line comments") {
		auto const expected = std::vector<token>{num{"1"}, plus{}, num{"2"}};
		auto const actual = lex("1+2 // This is a line comment.");
		CHECK(expected == actual.value());
	}

	TEST_CASE("key words") {
		// clang-format off
		auto const expected = std::vector<token>{
			imp{}, tok::num{"1"}, sym{"imports"},
			if_{}, tok::num{"1"}, sym{"ifs"},
			then{}, tok::num{"1"}, sym{"thens"},
			else_{}, tok::num{"1"}, sym{"elses"},
			while_{}, tok::num{"1"}, sym{"whiles"},
			for_{}, tok::num{"1"}, sym{"fors"},
			in{}, tok::num{"1"}, sym{"ins"},
			do_{}, tok::num{"1"}, sym{"dos"},
			ret{}, tok::num{"1"}, sym{"returns"},
			and_{}, tok::num{"1"}, sym{"ands"},
			or_{}, tok::num{"1"}, sym{"ors"},
			not_{}, tok::num{"1"}, sym{"nots"},
			};
		// clang-format on
		auto const actual = lex(R"(
			import1 imports
			if1 ifs
			then1 thens
			else1 elses
			while1 whiles
			for1 fors
			in1 ins
			do1 dos
			return1 returns
			and1 ands
			or1 ors
			not1 nots
			)");
		CHECK(expected == actual.value());
	}

	TEST_CASE("strings") {
		SUBCASE("valid") {
			CHECK(token{""} == lex(R"...("")...").value().front());
			CHECK(token{"abc"} == lex(R"...("abc")...").value().front());
			CHECK(token{R"...("abc")..."} == lex(R"...("\"abc\"")...").value().front());
			CHECK(token{R"...(a\b\c)..."} == lex(R"...("a\\b\\c")...").value().front());
		}
		SUBCASE("invalid") {
			CHECK(!lex(R"...(")...").has_value());
			CHECK(!lex(R"...(""")...").has_value());
			CHECK(!lex(R"...("\")...").has_value());
			CHECK(!lex(R"...("\a")...").has_value());
		}
	}
}
