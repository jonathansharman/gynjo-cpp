//! @file
//! @copyright See <a href="LICENSE.txt">LICENSE.txt</a>.
//! @brief Very simple logging utility.

#pragma once

#include <fmt/format.h>
#include <fmt/ostream.h>

namespace gynjo {
	template <typename... Args>
	auto log(Args&&... args) {
#ifdef _DEBUG
		fmt::print(args...);
#else
		// args unused in release mode.
		((void)args, ...);
#endif
	}
}
