//! @file
//! @copyright See <a href="LICENSE.txt">LICENSE.txt</a>.

#pragma once

#include "parser.hpp"
#include "values.hpp"

#include <tl/expected.hpp>

#include <string>
#include <unordered_map>

namespace gynjo {
	using eval_result = tl::expected<val::val, std::string>;
	using environment = std::unordered_map<std::string, val::val>;

	auto eval(environment& env, std::string const& input) -> eval_result;
}
