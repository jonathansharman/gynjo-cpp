//! @file
//! @copyright See <a href="LICENSE.txt">LICENSE.txt</a>.

#include "interpreter.hpp"

#ifndef _DEBUG
#define DOCTEST_CONFIG_DISABLE
#endif
#include <doctest/doctest.h>

TEST_SUITE("interpreter") {
	using namespace gynjo;

	TEST_CASE("empty statement") {
		auto env = environment::make();
		val::value const expected = val::make_list();
		auto const actual = eval(env, "");
		CHECK(expected == actual.value());
	}

	TEST_CASE("logical operators") {
		auto env = environment::make();
		val::value const t = tok::boolean{true};
		val::value const f = tok::boolean{false};
		SUBCASE("and") {
			CHECK(f == eval(env, "false and false").value());
			CHECK(f == eval(env, "false and true").value());
			CHECK(f == eval(env, "true and false").value());
			CHECK(t == eval(env, "true and true").value());
		}
		SUBCASE("or") {
			CHECK(f == eval(env, "false or false").value());
			CHECK(t == eval(env, "false or true").value());
			CHECK(t == eval(env, "true or false").value());
			CHECK(t == eval(env, "true or true").value());
		}
		SUBCASE("not") {
			CHECK(f == eval(env, "not true").value());
			CHECK(t == eval(env, "not false").value());
			CHECK(t == eval(env, "not false and false").value());
		}
		SUBCASE("short-circuiting") {
			CHECK(f == eval(env, "false and 1/0").value());
			CHECK(t == eval(env, "true or 1/0").value());
		}
		SUBCASE("and precedes or") {
			CHECK(t == eval(env, "true or true and false").value());
			CHECK(t == eval(env, "false and true or true").value());
		}
		SUBCASE("parenthesized") {
			CHECK(f == eval(env, "(false) and true").value());
			CHECK(t == eval(env, "false or ((true))").value());
			CHECK(t == eval(env, "not (false)").value());
		}
	}

	TEST_CASE("comparisons") {
		auto env = environment::make();
		val::value const t = tok::boolean{true};
		val::value const f = tok::boolean{false};
		SUBCASE("==") {
			CHECK(t == eval(env, "1 == 1").value());
			CHECK(f == eval(env, "1 == 2").value());
		}
		SUBCASE("!=") {
			CHECK(t == eval(env, "1 != 2").value());
			CHECK(f == eval(env, "1 != 1").value());
		}
		SUBCASE("<") {
			CHECK(t == eval(env, "1 < 2").value());
			CHECK(f == eval(env, "1 < 1").value());
			CHECK(f == eval(env, "2 < 1").value());
		}
		SUBCASE("<=") {
			CHECK(t == eval(env, "1 <= 2").value());
			CHECK(t == eval(env, "1 <= 1").value());
			CHECK(f == eval(env, "2 <= 1").value());
		}
		SUBCASE(">") {
			CHECK(f == eval(env, "1 > 2").value());
			CHECK(f == eval(env, "1 > 1").value());
			CHECK(t == eval(env, "2 > 1").value());
		}
		SUBCASE(">=") {
			CHECK(f == eval(env, "1 >= 2").value());
			CHECK(t == eval(env, "1 >= 1").value());
			CHECK(t == eval(env, "2 >= 1").value());
		}
		SUBCASE("comparisons and logical operators") {
			CHECK(t == eval(env, "1 == 1 and 2 == 2").value());
			CHECK(f == eval(env, "1 == 1 and 2 == 3").value());
			CHECK(t == eval(env, "1 == 2 or 3 == 3").value());
			CHECK(f == eval(env, "1 == 2 or 3 == 4").value());
		}
		SUBCASE("non-numbers are equal-checkable but not comparable") {
			// Booleans and tuples can be equality-checked.
			CHECK(t == eval(env, "true == true").value());
			CHECK(f == eval(env, "true == false").value());
			CHECK(f == eval(env, "true != true").value());
			CHECK(t == eval(env, "true != false").value());
			CHECK(t == eval(env, "(1, 2, 3) == (1, 2, 3)").value());
			CHECK(f == eval(env, "(1, 2, 3) == (3, 2, 1)").value());
			CHECK(f == eval(env, "(1, 2, 3) != (1, 2, 3)").value());
			CHECK(t == eval(env, "(1, 2, 3) != (3, 2, 1)").value());
			// Cannot be compared.
			CHECK(!eval(env, "false < true").has_value());
			CHECK(!eval(env, "(1, 2, 3) < (3, 2, 1)").has_value());
		}
		SUBCASE("different types cannot be compared") {
			CHECK(!eval(env, "true == (1, 2, 3)").has_value());
			CHECK(!eval(env, "(x -> x) < false").has_value());
		}
		SUBCASE("comparison precedes equality") {
			CHECK(t == eval(env, "1 < 2 == 2 < 3").value());
			CHECK(t == eval(env, "1 > 2 != 2 < 3").value());
		}
		SUBCASE("parenthesized") {
			CHECK(t == eval(env, "(1) == 1").value());
			CHECK(f == eval(env, "1 != (1)").value());
			CHECK(t == eval(env, "((1)) < 2").value());
			CHECK(t == eval(env, "1 <= ((1))").value());
			CHECK(f == eval(env, "(1) > (1)").value());
			CHECK(f == eval(env, "((1)) >= (2)").value());
		}
	}

	TEST_CASE("conditional expressions") {
		auto env = environment::make();
		SUBCASE("true is lazy") {
			val::value const expected = 1;
			auto const actual = eval(env, "if false then 1/0 else 1");
			CHECK(expected == actual.value());
		}
		SUBCASE("false is lazy") {
			val::value const expected = 1;
			auto const actual = eval(env, "if true then 1 else 1/0");
			CHECK(expected == actual.value());
		}
		SUBCASE("no else expression") {
			val::value const expected = val::make_list();
			auto const actual = eval(env, "if false then 1");
			CHECK(expected == actual.value());
		}
	}

	TEST_CASE("subtraction and negation") {
		auto env = environment::make();
		val::value const expected = val::num{-1.25};
		auto const actual = eval(env, "-1+-2^-2");
		CHECK(expected == actual.value());
	}

	TEST_CASE("simple compound with parentheses") {
		auto env = environment::make();
		val::value const expected = val::num{5.0};
		auto const actual = eval(env, "-5 *(1 +  -2)");
		CHECK(expected == actual.value());
	}

	TEST_CASE("basic assignment") {
		auto env = environment::make();
		val::value const expected = val::num{42};
		auto const actual = eval(env, "x = 42");
		CHECK(expected == actual.value());
	}

	TEST_CASE("tuple evaluation") {
		auto env = environment::make();
		SUBCASE("nested tuple of numbers") {
			val::value const expected = val::make_list(val::num{1}, val::make_list(val::num{2}, val::num{3}));
			auto const actual = eval(env, "(1, (2, 3))");
			CHECK(expected == actual.value());
		}
		SUBCASE("nested tuple of numbers and booleans") {
			val::value const expected = val::make_list(tok::boolean{true}, val::make_list(val::num{2}, tok::boolean{false}));
			auto const actual = eval(env, "(1 < 2, (2, false))");
			CHECK(expected == actual.value());
		}
	}

	TEST_CASE("simple function application") {
		auto env = environment::make();
		eval(env, "f = () -> 42");
		val::value const expected = val::num{42};
		auto const actual = eval(env, "f()");
		CHECK(expected == actual.value());
	}

	TEST_CASE("order of operations") {
		auto env = environment::make();
		eval(env, "inc = a -> a + 1");
		SUBCASE("parenthesized function call before exponentiation") {
			val::value const expected = val::num{36};
			auto const actual = eval(env, "4inc(2)^2");
			CHECK(expected == actual.value());
		}
		SUBCASE("exponentiation before non-parenthesized function call") {
			val::value const expected = val::num{20};
			auto const actual = eval(env, "4inc 2^2");
			CHECK(expected == actual.value());
		}
	}

	TEST_CASE("higher-order functions") {
		auto env = environment::make();
		val::value const expected = val::num{42};
		eval(env, "apply = (f, a) -> f(a)");
		auto const actual = eval(env, "apply(a -> a, 42)");
		CHECK(expected == actual.value());
	}

	TEST_CASE("curried function") {
		auto env = environment::make();
		val::value const expected = val::num{3};
		eval(env, "sum = a -> b -> a + b");
		auto const actual = eval(env, "sum 1 2");
		CHECK(expected == actual.value());
	}

	TEST_CASE("environment doesn't persist between function chains") {
		auto env = environment::make();
		eval(env, "sum = a -> b -> a + b");
		eval(env, "get_a = () -> a");
		auto const result = eval(env, "sum (1) (2) get_a ()");
		// a should be undefined.
		CHECK(!result.has_value());
	}

	TEST_CASE("chained application with and without parentheses") {
		auto env = environment::make();
		val::value const expected = val::num{3};
		eval(env, "sum = a -> b -> a + b");
		eval(env, "inc = a -> a + 1");
		auto const actual = eval(env, "sum (1) 2");
		CHECK(expected == actual.value());
	}

	TEST_CASE("chained application does not pollute applications higher in the call chain") {
		auto env = environment::make();
		val::value const expected = val::num{8};
		eval(env, "sum = a -> b -> a + b");
		eval(env, "inc = b -> b + 1");
		auto const actual = eval(env, "sum (inc 5) 2");
		CHECK(expected == actual.value());
	}

	TEST_CASE("importing standard constants") {
		auto env = environment::make();
		val::value const expected = val::num{
			"3.1415926535897932384626433832795028841971693993751058209749445923078164062862089986280348253421170679"};
		eval(env, "import constants");
		auto const actual = eval(env, "PI");
		CHECK(expected == actual.value());
	}
}
