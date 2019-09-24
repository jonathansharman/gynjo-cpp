//! @file
//! @copyright See <a href="LICENSE.txt">LICENSE.txt</a>.

#pragma once

#include "parser.hpp"

#include <tl/expected.hpp>

#include <string>

namespace gynjo {
	using eval_result = tl::expected<double, std::string>;

	auto eval(std::string const& input) -> eval_result;
}
