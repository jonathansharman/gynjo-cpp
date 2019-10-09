//! @file
//! @copyright See <a href="LICENSE.txt">LICENSE.txt</a>.

#include "interpreter.hpp"

#ifndef _DEBUG
#define DOCTEST_CONFIG_DISABLE
#endif
#include <doctest/doctest.h>

TEST_SUITE("core libraries") {
	using namespace gynjo;

	auto env = [] {
		auto result = environment::make();
#ifdef _DEBUG
		load_core_libs(result);
#endif
		return result;
	}();

	TEST_CASE("basic math") {
		SUBCASE("absolute value") {
			CHECK(val::value{val::num{5}} == eval(env, "abs 5").value());
			CHECK(val::value{val::num{5}} == eval(env, "abs(-5)").value());
		}
	}

	TEST_CASE("combinatorics") {
		SUBCASE("factorial") {
			CHECK(val::value{val::num{120}} == eval(env, "fact 5").value());
		}
		SUBCASE("permutations") {
			CHECK(val::value{val::num{60}} == eval(env, "nPk(5, 3)").value());
		}
		SUBCASE("combinations") {
			// Using an epsilon here because of the division.
			CHECK(val::value{tok::boolean{true}} == eval(env, "abs(nCk(5, 3) - 10) < 10**-50").value());
		}
	}
	TEST_CASE("list operations") {
		SUBCASE("len") {
			CHECK(val::value{val::num{0}} == eval(env, "len []").value());
			CHECK(val::value{val::num{3}} == eval(env, "len [1, 2, 3]").value());
		}
		SUBCASE("nth") {
			CHECK(!eval(env, "nth([], 0)").has_value());
			CHECK(val::value{val::num{2}} == eval(env, "nth([1, 2, 3], 1)").value());
		}
		SUBCASE("append") {
			val::value const expected = val::make_list(4, 3, 2, 1);
			auto const actual = eval(env, "append([1, 2, 3], 4)");
			CHECK(expected == actual.value());
		}
		SUBCASE("reverse") {
			val::value const expected = val::make_list(1, 2, 3);
			auto const actual = eval(env, "reverse [1, 2, 3]");
			CHECK(expected == actual.value());
		}
		SUBCASE("concat") {
			val::value const expected = val::make_list(4, 3, 2, 1);
			auto const actual = eval(env, "concat([1, 2], [3, 4])");
			CHECK(expected == actual.value());
		}
		SUBCASE("insert") {
			val::value const expected = val::make_list(3, 2, 1);
			auto const actual = eval(env, "insert([1, 3], 1, 2)");
			CHECK(expected == actual.value());
		}
		SUBCASE("remove") {
			val::value const expected = val::make_list(3, 1);
			auto const actual = eval(env, "remove([1, 2, 3], 1)");
			CHECK(expected == actual.value());
		}
		SUBCASE("map") {
			val::value const expected = val::make_list(9, 4, 1);
			auto const actual = eval(env, "map([1, 2, 3], x -> x^2)");
			CHECK(expected == actual.value());
		}
		SUBCASE("reduce") {
			val::value const expected = val::num{6};
			auto const actual = eval(env, "reduce([1, 2, 3], 0, (a, b) -> a + b)");
			CHECK(expected == actual.value());
		}
		SUBCASE("flatmap") {
			val::value const expected = val::make_list(3, 3, 2, 2, 1, 1);
			auto const actual = eval(env, "flatmap([1, 2, 3], x -> [x, x])");
			CHECK(expected == actual.value());
		}
	}
}
