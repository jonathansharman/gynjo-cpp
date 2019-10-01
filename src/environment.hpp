//! @file
//! @copyright See <a href="LICENSE.txt">LICENSE.txt</a>.

#pragma once

#include "values.hpp"

#include <string>
#include <unordered_map>

namespace gynjo {
	//! Contains a map from symbol names to their current Gynjo values.
	struct environment {
		std::unordered_map<std::string, val::value> vars;

		bool operator==(environment const&) const noexcept = default;
	};
}
