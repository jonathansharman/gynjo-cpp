//! @file
//! @copyright See <a href="LICENSE.txt">LICENSE.txt</a>.

#include "parser.hpp"

#ifndef _DEBUG
#define DOCTEST_CONFIG_DISABLE
#endif
#include <doctest/doctest.h>

TEST_SUITE("parser") {
	using namespace gynjo;

	TEST_CASE("kitchen sink") {
		auto factors = std::make_unique<std::vector<ast::node>>();
		factors->push_back(ast::make_tup(tok::num{"1"}, tok::num{"2"}));
		factors->push_back(ast::add{//
			make_node(ast::node{tok::num{"3"}}),
			make_node(ast::node{tok::num{"4"}})});
		auto const expected = //
			make_node(ast::assign{
				tok::sym{"f"},
				make_node(ast::lambda{
					make_node(ast::tup{}),
					make_node(ast::neg{
						make_node(ast::cluster{
							std::move(factors), {ast::cluster::connector::mul} //
						}) //
					}) //
				}) //
			});

		auto const actual = parse(std::vector<tok::token>{//
			tok::sym{"f"},
			tok::eq{},
			tok::lft{},
			tok::rht{},
			tok::arrow{},
			tok::minus{},
			tok::lft{},
			tok::num{"1"},
			tok::com{},
			tok::num{"2"},
			tok::rht{},
			tok::mul{},
			tok::lft{},
			tok::num{"3"},
			tok::plus{},
			tok::num{"4"},
			tok::rht{}});

		CHECK(to_string(*expected) == to_string(actual.value()));
	}
}
