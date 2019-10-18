//! @file
//! @copyright See <a href="LICENSE.txt">LICENSE.txt</a>.

#include "interpreter.hpp"

#ifndef _DEBUG
#define DOCTEST_CONFIG_DISABLE
#endif
#define DOCTEST_CONFIG_IMPLEMENT
#include <doctest/doctest.h>

#include <array>
#include <iostream>
#include <string>

auto main(int argc, char* argv[]) -> int {
#ifdef _DEBUG
	doctest::Context context;
	context.setOption("no-breaks", true);
	context.setOption("success", false);
	context.applyCommandLine(argc, argv);
	context.run();
#else
	// Command-line arguments unused in release build. Supress unused variable warnings.
	(void)argc;
	(void)argv;
#endif

	using namespace gynjo;

	auto env = environment::make_with_core_libs();
	for (;;) {
		std::cout << ">> ";
		std::string input;
		std::getline(std::cin, input);
		while (!input.empty() && input.back() == '\\') {
			// Continue line. Add a space to ensure new token on next line.
			input.back() = ' ';
			std::cout << "   ";
			std::string next_line;
			std::getline(std::cin, next_line);
			input += next_line;
		}
		// First try to interpret the line as an expression.
		auto eval_result = eval(env, input);
		if (eval_result.has_value()) {
			// Print the computed value.
			fmt::print("{}\n", val::to_string(eval_result.value()));
		} else {
			// Invalid expression. Try a statement instead.
			auto exec_result = exec(env, input);
			// Still didn't work; report statement error.
			if (!exec_result.has_value()) { fmt::print("{}\n", exec_result.error()); }
		}
	}
}
