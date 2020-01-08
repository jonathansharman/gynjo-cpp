//! @file
//! @copyright See <a href="LICENSE.txt">LICENSE.txt</a>.

#pragma once

#include "environment.hpp"
#include "parser.hpp"
#include "values.hpp"

#include <tl/expected.hpp>

namespace gynjo {
	//! Result of evaluation: either a Gynjo value or an error message.
	using eval_result = tl::expected<val::value, std::string>;

	//! Result of execution: either a @p std::monostate or an error message.
	using exec_result = tl::expected<std::monostate, std::string>;

	//! If possible, computes the value of @p expr in the context of @env.
	auto eval(env_ptr const& env, expr const& expr) -> eval_result;

	//! If possible, computes the value of the expression contained in @p input in the context of @env.
	auto eval(env_ptr const& env, std::string const& input) -> eval_result;

	//! If possible, executes @p stmt in the context of @env.
	auto exec(env_ptr const& env, stmt const& stmt) -> exec_result;

	//! If possible, executes the statements contained in @p input in the context of @env.
	auto exec(env_ptr const& env, std::string const& input) -> exec_result;
}
