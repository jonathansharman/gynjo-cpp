//! @file
//! @copyright See <a href="LICENSE.txt">LICENSE.txt</a>.

#pragma once

#include "environment_fwd.hpp"

#include "values.hpp"

#include <memory>
#include <string>
#include <unordered_map>

namespace gynjo {
	struct environment {
		//! Shared pointer to environment, for convenience.
		using ptr = std::shared_ptr<environment>;

		//! Convenience factory method for creating a new empty shared environment pointer.
		static auto make() -> ptr;

		//! Variables mappings created within the local scope.
		std::unordered_map<std::string, val::value> local_vars;

		//! A pointer to the parent environment, if any.
		std::shared_ptr<environment> parent_env;

		//! @param parent_env A pointer to the parent environment, if any.
		environment(ptr parent_env = nullptr);

		//! Returns the value of the variable with name @name or nullopt if the variable is undefined.
		auto lookup(std::string const& name) -> std::optional<val::value> const;
	};
}
