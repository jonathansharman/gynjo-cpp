//! @file
//! @copyright See <a href="LICENSE.txt">LICENSE.txt</a>.

#pragma once

#include "values.hpp"

#include <memory>
#include <string>
#include <unordered_map>

namespace gynjo {
	//! Map symbol names to their current Gynjo values.
	struct environment {
		//! Shared pointer to environment, for convenience.
		using ptr = std::shared_ptr<environment>;

		//! Convenience factory method for creating a new empty shared environment pointer.
		static auto make() -> ptr {
			return std::make_shared<environment>();
		}

		//! Variables mappings created within the local scope.
		std::unordered_map<std::string, val::value> local_vars;

		//! A pointer to the parent environment, if any.
		std::shared_ptr<environment> parent_env;

		//! @param parent_env A pointer to the parent environment, if any.
		environment(std::shared_ptr<environment> parent_env = nullptr) : parent_env{std::move(parent_env)} {}

		//! Returns the value of the variable with name @name or nullopt if the variable is undefined.
		auto lookup(std::string const& name) -> std::optional<val::value> const {
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
	};
}
