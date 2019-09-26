//! @file
//! @copyright See <a href="LICENSE.txt">LICENSE.txt</a>.

#pragma once

#include "tokens.hpp"
#include "visitation.hpp"

#include <boost/multiprecision/cpp_dec_float.hpp>

#include <memory>

namespace gynjo::val {
	using num = boost::multiprecision::cpp_dec_float_100;

	using val = std::variant<num>;

	inline auto to_string(val const& val) -> std::string {
		return match(val, [](num const& num) { return num.str(); });
	}
}
