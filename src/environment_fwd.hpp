//! @file
//! @copyright See <a href="LICENSE.txt">LICENSE.txt</a>.

#pragma once

#include <memory>

namespace gynjo {
	struct environment;

	//! Shared pointer to an environment, for convenience.
	using env_ptr = std::shared_ptr<environment>;
}
