//! @file
//! @copyright See <a href="LICENSE.txt">LICENSE.txt</a>.

#include "environment.hpp"

#include "interpreter.hpp"

namespace gynjo {
	auto environment::make_empty() -> ptr {
		return std::make_shared<environment>();
	}

	auto environment::make_with_core_libs() -> ptr {
		static auto result = [] {
			auto env = std::make_shared<environment>();
			import_lib(env, "\"core/constants.gynj\"");
			import_lib(env, "\"core/core.gynj\"");
			return env;
		}();
		return result;
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

	auto import_lib(environment::ptr const& env, std::string_view lib) -> void {
		auto import_result = exec(env, fmt::format("import {}", lib));
		if (!import_result.has_value()) { fmt::print("Error while importing {}: {}\n", lib, import_result.error()); }
	}
}
