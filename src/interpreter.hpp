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

	//! If possible, computes the value of the expression @p node in the context of @env.
	auto eval(environment::ptr const& env, ast::node const& node) -> eval_result;

	//! If possible, computes the value of the expression contained in @p input in the context of @env.
	auto eval(environment::ptr const& env, std::string const& input) -> eval_result;

	//! If possible, executes the statements contained in the AST @p node in the context of @env.
	auto exec(environment::ptr const& env, ast::node const& node) -> exec_result;

	//! If possible, executes the statements contained in @p input in the context of @env.
	auto exec(environment::ptr const& env, std::string const& input) -> exec_result;
}
