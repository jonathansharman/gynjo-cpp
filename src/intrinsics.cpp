//! @file
//! @copyright See <a href="LICENSE.txt">LICENSE.txt</a>.

#include "intrinsics.hpp"

namespace gynjo {
	auto name(intrinsic f) -> std::string {
		switch (f) {
			case intrinsic::len:
				return "len";
			case intrinsic::at:
				return "at";
			case intrinsic::push:
				return "push";
			case intrinsic::pop:
				return "pop";
			case intrinsic::insert:
				return "insert";
			case intrinsic::erase:
				return "erase";
			default:
				// unreachable
				return "unknown";
		}
	}

	auto arity(intrinsic f) -> std::size_t {
		switch (f) {
			case intrinsic::len:
				return 1;
			case intrinsic::at:
				return 2;
			case intrinsic::push:
				return 2;
			case intrinsic::pop:
				return 1;
			case intrinsic::insert:
				return 3;
			case intrinsic::erase:
				return 2;
			default:
				// unreachable
				return 0;
		}
	}
}
