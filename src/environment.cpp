//! @file
//! @copyright See <a href="LICENSE.txt">LICENSE.txt</a>.

#include "environment.hpp"

#include "interpreter.hpp"

namespace gynjo {
	auto environment::make_empty() -> env_ptr {
		return std::make_shared<environment>();
	}

	auto environment::make_with_core_libs() -> env_ptr {
		static auto result = [] {
			auto env = std::make_shared<environment>();
			import_lib(env, "\"core/constants.gynj\"");
			import_lib(env, "\"core/core.gynj\"");
			return env;
		}();
		return result;
	}

	environment::environment(env_ptr parent_env) : parent_env{std::move(parent_env)} {}

	auto environment::lookup(std::string_view name) -> std::optional<val::value> const {
		// For now, this is the best way I can think of to do a string_view lookup without allocating.
		// In C++20, will be able to just use find(), thanks to P0919R2: Heterogeneous lookup for unordered containers.
		std::unordered_map<std::string, val::value>::value_type x;
		auto const it = std::find_if(
			local_vars.begin(), local_vars.end(), [&](auto const& name_val) { return name_val.first == name; });
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

	auto import_lib(env_ptr const& env, std::string_view lib) -> void {
		auto import_result = exec(env, fmt::format("import {}", lib));
		if (!import_result.has_value()) { fmt::print("Error while importing {}: {}\n", lib, import_result.error()); }
	}
}
