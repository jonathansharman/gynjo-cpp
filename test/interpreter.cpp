//! @file
//! @copyright See <a href="LICENSE.txt">LICENSE.txt</a>.

#include "interpreter.hpp"

#ifndef _DEBUG
#define DOCTEST_CONFIG_DISABLE
#endif
#include <doctest/doctest.h>

#include <sstream>

TEST_SUITE("interpreter") {
	using namespace gynjo;

	TEST_CASE("execution of empty statement does nothing") {
		auto env = environment::make_empty();
		auto const actual = exec(env, "");
		CHECK(std::monostate{} == actual.value());
	}

	TEST_CASE("logical operators") {
		auto env = environment::make_empty();
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
		auto env = environment::make_empty();
		val::value const t = tok::boolean{true};
		val::value const f = tok::boolean{false};
		SUBCASE("==") {
			CHECK(t == eval(env, "1 = 1").value());
			CHECK(f == eval(env, "1 = 2").value());
		}
		SUBCASE("!=") {
			CHECK(t == eval(env, "1 != 2").value());
			CHECK(f == eval(env, "1 != 1").value());
		}
		SUBCASE("~") {
			CHECK(t == eval(env, "1/3 ~ 0.333333333333").value());
			CHECK(f == eval(env, "1/3 ~ 0.333").value());
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
			CHECK(t == eval(env, "1 = 1 and 2 = 2").value());
			CHECK(f == eval(env, "1 = 1 and 2 = 3").value());
			CHECK(t == eval(env, "1 = 2 or 3 = 3").value());
			CHECK(f == eval(env, "1 = 2 or 3 = 4").value());
		}
		SUBCASE("non-numbers are equal-checkable but not comparable") {
			// Booleans and tuples can be equality-checked.
			CHECK(t == eval(env, "true = true").value());
			CHECK(f == eval(env, "true = false").value());
			CHECK(f == eval(env, "true != true").value());
			CHECK(t == eval(env, "true != false").value());
			CHECK(t == eval(env, "(1, 2, 3) = (1, 2, 3)").value());
			CHECK(f == eval(env, "(1, 2, 3) = (3, 2, 1)").value());
			CHECK(f == eval(env, "(1, 2, 3) != (1, 2, 3)").value());
			CHECK(t == eval(env, "(1, 2, 3) != (3, 2, 1)").value());
			// Cannot be compared.
			CHECK(!eval(env, "false < true").has_value());
			CHECK(!eval(env, "(1, 2, 3) < (3, 2, 1)").has_value());
		}
		SUBCASE("different types compare inequal") {
			CHECK(f == eval(env, "[1, 2, 3] = (1, 2, 3)").value());
			CHECK(t == eval(env, "(x -> x) != false").value());
		}
		SUBCASE("different types cannot be order-compared") {
			CHECK(!eval(env, "(x -> x) < false").has_value());
		}
		SUBCASE("comparison precedes equality") {
			CHECK(t == eval(env, "1 < 2 = 2 < 3").value());
			CHECK(t == eval(env, "1 > 2 != 2 < 3").value());
		}
		SUBCASE("parenthesized") {
			CHECK(t == eval(env, "(1) = 1").value());
			CHECK(f == eval(env, "1 != (1)").value());
			CHECK(t == eval(env, "((1)) < 2").value());
			CHECK(t == eval(env, "1 <= ((1))").value());
			CHECK(f == eval(env, "(1) > (1)").value());
			CHECK(f == eval(env, "((1)) >= (2)").value());
		}
	}

	TEST_CASE("conditional expressions") {
		auto env = environment::make_empty();
		SUBCASE("true is lazy") {
			val::value const expected = 1;
			auto const actual = eval(env, "false ? 1/0 : 1");
			CHECK(expected == actual.value());
		}
		SUBCASE("false is lazy") {
			val::value const expected = 1;
			auto const actual = eval(env, "true ? 1 : 1/0");
			CHECK(expected == actual.value());
		}
	}

	TEST_CASE("subtraction and negation") {
		auto env = environment::make_empty();
		val::value const expected = val::num{-1.25};
		auto const actual = eval(env, "-1+-2^-2");
		CHECK(expected == actual.value());
	}

	TEST_CASE("simple compound expression with parentheses") {
		auto env = environment::make_empty();
		val::value const expected = val::num{5.0};
		auto const actual = eval(env, "-5 *(1 +  -2)");
		CHECK(expected == actual.value());
	}

	TEST_CASE("exponentiation is right-associative") {
		auto env = environment::make_empty();
		val::value const expected = val::num{262144};
		auto const actual = eval(env, "4^3^2");
		CHECK(expected == actual.value());
	}

	TEST_CASE("basic assignment") {
		auto env = environment::make_empty();
		val::value const expected = val::num{42};
		exec(env, "let x = 42");
		auto const actual = eval(env, "x");
		CHECK(expected == actual.value());
	}

	TEST_CASE("tuples") {
		auto env = environment::make_empty();
		SUBCASE("singleton collapses into contained value") {
			val::value const expected = val::num{1};
			auto const actual = eval(env, "(1)");
			CHECK(expected == actual.value());
		}
		SUBCASE("nested tuple of numbers") {
			val::value const expected = val::make_tup(val::num{1}, val::make_tup(val::num{2}, val::num{3}));
			auto const actual = eval(env, "(1, (2, 3))");
			CHECK(expected == actual.value());
		}
		SUBCASE("nested tuple of numbers and booleans") {
			val::value const expected = val::make_tup(tok::boolean{true}, val::make_tup(val::num{2}, tok::boolean{false}));
			auto const actual = eval(env, "(1 < 2, (2, false))");
			CHECK(expected == actual.value());
		}
	}

	TEST_CASE("list construction") {
		auto env = environment::make_empty();
		SUBCASE("singleton list") {
			val::value const expected = val::make_list(val::num{1});
			auto const actual = eval(env, "[1]");
			CHECK(expected == actual.value());
		}
		SUBCASE("nested list of numbers") {
			val::value const expected = val::make_list(val::make_list(val::num{3}, val::num{2}), val::num{1});
			auto const actual = eval(env, "[1, [2, 3]]");
			CHECK(expected == actual.value());
		}
		SUBCASE("nested list of numbers and booleans") {
			val::value const expected = val::make_list(val::make_list(tok::boolean{false}, val::num{2}), tok::boolean{true});
			auto const actual = eval(env, "[1 < 2, [2, false]]");
			CHECK(expected == actual.value());
		}
	}

	TEST_CASE("list destruction does not cause stack overflow") {
		auto env = environment::make_empty();
		auto result = exec(env, R"(
			let i = 0
			let l = []
			while i < 1000 do {
				let l = push(l, i)
				let i = i + 1
			};
			)");
		CHECK(result.has_value());
	}

	TEST_CASE("math operations on lists") {
		auto env = environment::make_empty();
		SUBCASE("addition") {
			val::value const expected = val::make_list(val::num{4}, val::num{3}, val::num{2});
			auto const actual1 = eval(env, "[1, 2, 3] + 1");
			CHECK(expected == actual1.value());
			auto const actual2 = eval(env, "1 + [1, 2, 3]");
			CHECK(expected == actual2.value());
		}
		SUBCASE("subtraction") {
			val::value const expected1 = val::make_list(val::num{3}, val::num{2}, val::num{1});
			auto const actual1 = eval(env, "[2, 3, 4]-1");
			CHECK(expected1 == actual1.value());
			val::value const expected2 = val::make_list(val::num{1}, val::num{2}, val::num{3});
			auto const actual2 = eval(env, "4-[1, 2, 3]");
			CHECK(expected2 == actual2.value());
		}
		SUBCASE("multiplication") {
			val::value const expected = val::make_list(val::num{6}, val::num{4}, val::num{2});
			auto const actual1 = eval(env, "[1, 2, 3]2");
			CHECK(expected == actual1.value());
			auto const actual2 = eval(env, "2[1, 2, 3]");
			CHECK(expected == actual2.value());
		}
		SUBCASE("division") {
			val::value const expected1 = val::make_list(val::num{3}, val::num{2}, val::num{1});
			auto const actual1 = eval(env, "[2, 4, 6]/2");
			CHECK(expected1 == actual1.value());
			val::value const expected2 = val::make_list(val::num{2}, val::num{3}, val::num{6});
			auto const actual2 = eval(env, "6/[1, 2, 3]");
			CHECK(expected2 == actual2.value());
		}
		SUBCASE("exponentiation") {
			val::value const expected1 = val::make_list(val::num{9}, val::num{4}, val::num{1});
			auto const actual1 = eval(env, "[1, 2, 3]^2");
			CHECK(expected1 == actual1.value());
			val::value const expected2 = val::make_list(val::num{8}, val::num{4}, val::num{2});
			auto const actual2 = eval(env, "2^[1, 2, 3]");
			CHECK(expected2 == actual2.value());
		}
	}

	TEST_CASE("simple function application") {
		auto env = environment::make_empty();
		exec(env, "let f = () -> 42");
		val::value const expected = val::num{42};
		auto const actual = eval(env, "f()");
		CHECK(expected == actual.value());
	}

	TEST_CASE("order of operations") {
		auto env = environment::make_empty();
		exec(env, "let inc = a -> a + 1");
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
		auto env = environment::make_empty();
		val::value const expected = val::num{42};
		exec(env, "let apply = (f, a) -> f(a)");
		auto const actual = eval(env, "apply(a -> a, 42)");
		CHECK(expected == actual.value());
	}

	TEST_CASE("curried function") {
		auto env = environment::make_empty();
		val::value const expected = val::num{3};
		exec(env, "let sum = a -> b -> a + b");
		auto const actual = eval(env, "sum 1 2");
		CHECK(expected == actual.value());
	}

	TEST_CASE("environment doesn't persist between function chains") {
		auto env = environment::make_empty();
		exec(env, R"(
			let sum = a -> b -> a + b
			let get_a = () -> a
			)");
		auto const result = eval(env, "sum (1) (2) get_a ()");
		// a should be undefined.
		CHECK(!result.has_value());
	}

	TEST_CASE("chained application with and without parentheses") {
		auto env = environment::make_empty();
		val::value const expected = val::num{3};
		exec(env, R"(
			let sum = a -> b -> a + b
			let inc = a -> a + 1
			)");
		auto const actual = eval(env, "sum (1) 2");
		CHECK(expected == actual.value());
	}

	TEST_CASE("chained application does not pollute applications higher in the call chain") {
		auto env = environment::make_empty();
		val::value const expected = val::num{8};
		exec(env, R"(
			let sum = a -> b -> a + b
			let inc = b -> b + 1
			)");
		auto const actual = eval(env, "sum (inc 5) 2");
		CHECK(expected == actual.value());
	}

	TEST_CASE("importing core constants") {
		auto env = environment::make_empty();
		val::value const expected = val::num{
			"3.1415926535897932384626433832795028841971693993751058209749445923078164062862089986280348253421170679"};
		exec(env, "import \"core/constants.gynj\"");
		auto const actual = eval(env, "PI");
		CHECK(expected == actual.value());
	}

	TEST_CASE("blocks") {
		auto env = environment::make_empty();
		val::value const expected = val::num{"1"};
		eval(env, "{}");
		eval(env, "{ let a = 0 }");
		eval(env, "{ let b = { let a = a + 1 return a } }");
		auto const actual = eval(env, "b");
		CHECK(expected == actual.value());
	}

	TEST_CASE("branch statements") {
		auto env = environment::make_empty();
		SUBCASE("true is lazy") {
			val::value const expected = 1;
			exec(env, "if false then let a = 1/0 else let a = 1");
			auto const actual = eval(env, "a");
			CHECK(expected == actual.value());
		}
		SUBCASE("false is lazy") {
			val::value const expected = 1;
			exec(env, "if true then let a = 1 else let a = 1/0");
			auto const actual = eval(env, "a");
			CHECK(expected == actual.value());
		}
		SUBCASE("no else statement") {
			auto const actual = exec(env, "if false then let a = 1/0");
			CHECK(std::monostate{} == actual.value());
		}
	}

	TEST_CASE("while-loops") {
		auto env = environment::make_empty();
		val::value const expected = val::num{"3"};
		exec(env, R"(
			let a = 0
			while a < 3 do let a = a + 1
			)");
		auto const actual = eval(env, "a");
		CHECK(expected == actual.value());
	}

	TEST_CASE("for-loops") {
		auto env = environment::make_empty();
		val::value const expected = val::num{"6"};
		exec(env, R"(
			let a = 0
			for x in [1, 2, 3] do let a = a + x
			for x in [] do let a = 10
			)");
		auto const actual = eval(env, "a");
		CHECK(expected == actual.value());
	}

	TEST_CASE("Intrinsic I/O functions") {
		auto env = environment::make_empty();
		// Temporarily redirect cin and cout to string streams to allow control of input and observation of output.
		std::istringstream sin{"test"};
		auto cin_rdbuf = std::cin.rdbuf(sin.rdbuf());
		std::ostringstream sout;
		auto cout_rdbuf = std::cout.rdbuf(sout.rdbuf());
		// Execute print() and read().
		auto result = exec(env, "print(read());");
		// Reset cin and cout to normal.
		std::cin.rdbuf(cin_rdbuf);
		std::cout.rdbuf(cout_rdbuf);
		// Now that the status quo has been restored, check the results. Should be the test string in quotes with a newline.
		CHECK(result.has_value());
		CHECK("\"test\"\n" == sout.str());
	}
}
