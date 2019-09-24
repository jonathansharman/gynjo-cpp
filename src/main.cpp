//! @file
//! @copyright See <a href="LICENSE.txt">LICENSE.txt</a>.

#include "interpreter.hpp"

#ifndef _DEBUG
#define DOCTEST_CONFIG_DISABLE
#endif
#define DOCTEST_CONFIG_IMPLEMENT
#include <doctest/doctest.h>

#include <iostream>
#include <string>

auto main(int argc, char* argv[]) -> int {
	int main_result = 0;

#ifdef _DEBUG
	doctest::Context context;
	context.setOption("no-breaks", false);
	context.setOption("success", false);
	context.applyCommandLine(argc, argv);
	main_result = context.run();
#else
	// Command-line arguments unused in release build. Supress unused variable warnings.
	(void)argc;
	(void)argv;
#endif

	for (;;) {
		std::cout << ">> ";
		std::string line;
		std::getline(std::cin, line);
		if (line == "exit" || line == "quit") break;
		auto const eval_result = gynjo::eval(line);
		std::cout << (eval_result.has_value() ? std::to_string(eval_result.value()) : eval_result.error()) << '\n';
	}

	return main_result;
}
