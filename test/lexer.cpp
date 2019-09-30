//! @file
//! @copyright See <a href="LICENSE.txt">LICENSE.txt</a>.

#include "lexer.hpp"

#ifndef _DEBUG
#define DOCTEST_CONFIG_DISABLE
#endif
#include <doctest/doctest.h>

TEST_SUITE("lexer") {
	using namespace gynjo;

	TEST_CASE("kitchen sink") {
		auto const expected = std::vector<tok::token>{//
			tok::num{"5"},
			tok::mul{},
			tok::lft{},
			tok::num{"1"},
			tok::plus{},
			tok::minus{},
			tok::arrow{},
			tok::num{"2"},
			tok::rht{},
			tok::exp{},
			tok::exp{},
			tok::mul{},
			tok::eq{},
			tok::div{},
			tok::num{".1"},
			tok::num{"0"},
			tok::num{"0.1"},
			tok::com{}};

		auto const actual = lex("5*( 1+ \t -->2)^***  =/ .1 0 0.1,");

		CHECK(expected == actual.value());
	}
}
