//! @file
//! @copyright See <a href="LICENSE.txt">LICENSE.txt</a>.

#pragma once

#include "parser.hpp"

#include <tl/expected.hpp>

#include <string>
#include <unordered_map>

namespace gynjo {
	using eval_result = tl::expected<double, std::string>;
	using environment = std::unordered_map<std::string, double>;

	auto eval(environment& env, std::string const& input) -> eval_result;
}
