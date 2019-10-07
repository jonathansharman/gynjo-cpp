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

	// Load core libraries.
	auto env = environment::make();
	load_core_libs(env);

	// REPL.
	for (;;) {
		std::cout << ">> ";
		std::string line;
		std::getline(std::cin, line);
		print(eval(env, line));
	}
}
