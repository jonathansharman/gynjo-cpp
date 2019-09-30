//! @file
//! @copyright See <a href="LICENSE.txt">LICENSE.txt</a>.

#pragma once

#include "environment.hpp"
#include "parser.hpp"
#include "values.hpp"

#include <tl/expected.hpp>

namespace gynjo {
	//! Either a Gynjo value or an error message.
	using eval_result = tl::expected<val::value, std::string>;

	//! Computes the value of @p ast in the context of @env, if possible.
	auto eval(environment& env, ast::ptr const& ast) -> eval_result;

	//! Computes the value of @p input in the context of @env, if possible.
	auto eval(environment& env, std::string const& input) -> eval_result;
}
