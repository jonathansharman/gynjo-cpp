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
	context.setOption("no-breaks", false);
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
	environment env;
	auto core_libs = {"constants"};
	for (auto const& lib : core_libs) {
		fmt::print("Importing {}...\n", lib);
		print(eval(env, fmt::format("import {}", lib)));
	}
	fmt::print("Ready!\n");

	// REPL.
	for (;;) {
		std::cout << ">> ";
		std::string line;
		std::getline(std::cin, line);
		print(eval(env, line));
	}
}
