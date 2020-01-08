//! @file
//! @copyright See <a href="LICENSE.txt">LICENSE.txt</a>.

#pragma once

#include "environment_fwd.hpp"

#include "values.hpp"

#include <string>
#include <unordered_map>

namespace gynjo {
	struct environment {
		//! Convenience factory method for creating a new empty shared environment pointer.
		static auto make_empty() -> env_ptr;

		//! Convenience factory method for creating a new shared environment pointer with core libs loaded.
		static auto make_with_core_libs() -> env_ptr;

		//! Variables mappings created within the local scope.
		std::unordered_map<std::string, val::value> local_vars;

		//! A pointer to the parent environment, if any.
		std::shared_ptr<environment> parent_env;

		//! @param parent_env A pointer to the parent environment, if any.
		environment(env_ptr parent_env = nullptr);

		//! Returns the value of the variable with name @name or nullopt if the variable is undefined.
		auto lookup(std::string_view name) -> std::optional<val::value> const;
	};

	//! Attempts to import @p lib into @p env and displays an error message on failure.
	auto import_lib(env_ptr const& env, std::string_view lib) -> void;
}
