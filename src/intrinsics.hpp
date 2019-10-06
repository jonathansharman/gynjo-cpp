//! @file
//! @copyright See <a href="LICENSE.txt">LICENSE.txt</a>.

#pragma once

#include <string>

namespace gynjo {
	//! Represents an intrinsic Gynjo function or its body, depending on context.
	enum class intrinsic { len, at, push, pop, insert, erase };

	//! The user-readable name of intrinsic function @p f.
	auto name(intrinsic f) -> std::string;

	//! The required number of arguments to @p f.
	auto arity(intrinsic f) -> std::size_t;
}
