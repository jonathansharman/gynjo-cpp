//! @file
//! @copyright See <a href="LICENSE.txt">LICENSE.txt</a>.

#include "stmt.hpp"

#include "expr.hpp"

namespace gynjo {
	auto assign::operator==(assign const& that) const noexcept -> bool {
		return symbol == that.symbol && *rhs == *that.rhs;
	}

	auto branch::operator==(branch const& that) const noexcept -> bool {
		return *test == *that.test && *true_stmt == *that.true_stmt && *false_stmt == *that.false_stmt;
	}

	auto while_loop::operator==(while_loop const& that) const noexcept -> bool {
		return *test == *that.test && *body == *that.body;
	}

	auto for_loop::operator==(for_loop const& that) const noexcept -> bool {
		return loop_var == that.loop_var && *range == *that.range && *body == *that.body;
	}

	auto ret::operator==(ret const& that) const noexcept -> bool {
		return *result == *that.result;
	}

	auto expr_stmt::operator==(expr_stmt const& that) const noexcept -> bool {
		return *expr == *that.expr;
	}

	auto to_string(stmt const& stmt) -> std::string {
		using namespace std::string_literals;
		return match(
			stmt.value,
			[](nop) { return "no-op"s; },
			[](imp const& imp) { return "import " + imp.filename; },
			[](assign const& assign) {
				return fmt::format("let {} = {}", tok::to_string(assign.symbol), to_string(*assign.rhs));
			},
			[](branch const& branch) {
				return fmt::format(
					"if {} then {} else {}", to_string(*branch.test), to_string(*branch.true_stmt), to_string(*branch.false_stmt));
			},
			[](while_loop const& while_loop) {
				return fmt::format("while {} do {}", to_string(*while_loop.test), to_string(*while_loop.body));
			},
			[](for_loop const& for_loop) {
				return fmt::format("for {} in {} do {}",
					tok::to_string(for_loop.loop_var),
					to_string(*for_loop.range),
					to_string(*for_loop.body));
			},
			[](ret const& ret) { return fmt::format("return {}", to_string(*ret.result)); },
			[](expr_stmt const expr_stmt) { return fmt::format("{};", to_string(*expr_stmt.expr)); });
	}
}
