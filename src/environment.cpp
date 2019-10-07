//! @file
//! @copyright See <a href="LICENSE.txt">LICENSE.txt</a>.

#include "environment.hpp"

#include "interpreter.hpp"

namespace gynjo {
	auto environment::make() -> ptr {
		return std::make_shared<environment>();
	}

	environment::environment(environment::ptr parent_env) : parent_env{std::move(parent_env)} {}

	auto environment::lookup(std::string const& name) -> std::optional<val::value> const {
		auto it = local_vars.find(name);
		if (it != local_vars.end()) {
			// Found in local variables.
			return it->second;
		} else if (parent_env != nullptr) {
			// Try searching parent environment.
			return parent_env->lookup(name);
		} else {
			// Not found.
			return std::nullopt;
		}
	}

	auto load_core_libs(environment::ptr const& env) -> void {
		auto core_libs = {"constants", "core"};
		for (auto const& lib : core_libs) {
			fmt::print("Importing {}...\n", lib);
			print(eval(env, fmt::format("import {}", lib)));
		}
		fmt::print("Ready!\n");
	}
}
