//! @file
//! @copyright See <a href="LICENSE.txt">LICENSE.txt</a>.

#pragma once

#include <string>

namespace gynjo {
	//! Represents an intrinsic Gynjo function or its body, depending on context.
	enum class intrinsic {
		// Fundamental list operations
		top,
		pop,
		push,
		// I/O
		print,
		read
	};

	//! The user-readable name of intrinsic function @p f.
	auto name(intrinsic f) -> std::string;
}
