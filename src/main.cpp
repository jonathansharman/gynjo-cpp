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
	std::string line;
	for (;;) {
		std::cout << ">> ";
		std::getline(std::cin, line);
		auto exec_result = exec(env, line);
		// auto exec_result = exec(env, fmt::format("print({});", line));
		if (!exec_result.has_value()) { fmt::print("{}\n", exec_result.error()); }
	}
}
