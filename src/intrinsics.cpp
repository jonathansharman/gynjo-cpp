//! @file
//! @copyright See <a href="LICENSE.txt">LICENSE.txt</a>.

#include "intrinsics.hpp"

namespace gynjo {
	auto name(intrinsic f) -> std::string {
		switch (f) {
			case intrinsic::top:
				return "top";
			case intrinsic::pop:
				return "pop";
			case intrinsic::push:
				return "push";
			default:
				// unreachable
				return "unknown";
		}
	}
}
