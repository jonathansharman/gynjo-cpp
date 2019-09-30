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

	//! Computes the value of the abstract syntax tree @p ast in the context of @env, if possible.
	auto eval(environment& env, ast::node const& ast) -> eval_result;

	//! Computes the value of @p input in the context of @env, if possible.
	auto eval(environment& env, std::string const& input) -> eval_result;

	//! Prints the results of an evaluation (if not empty) or the error message.
	auto print(eval_result result) -> void;
}
