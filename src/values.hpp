//! @file
//! @copyright See <a href="LICENSE.txt">LICENSE.txt</a>.

#pragma once

#include "tokens.hpp"
#include "visitation.hpp"

#include <memory>

namespace gynjo::value {
	using val = std::variant<tok::num>;

	inline auto to_string(val const& value) -> std::string {
		return match(value, [](tok::num const& num) { return std::to_string(num.value); });
	}
}
