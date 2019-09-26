//! @file
//! @copyright See <a href="LICENSE.txt">LICENSE.txt</a>.
//! @brief Utility functions for making variant visitation nicer.

#pragma once

#include <variant>

namespace gynjo {
	//! Creates an overloaded function object from the given function types.
	template <typename... Funcs>
	struct overloaded : Funcs... {
		using Funcs::operator()...;
	};

	//! Creates an overloaded function object from the given function objects.
	template <typename... Funcs>
	overloaded(Funcs...)->overloaded<Funcs...>;

	//! Visits the variant @p v using the handlers @p handlers.
	//! @note Credit to Nikolai Wuttke, "std::variant and the power of pattern matching".
	template <typename Variant, typename... Handlers>
	auto match(Variant&& v, Handlers&&... handlers) {
		return std::visit(overloaded{std::forward<Handlers>(handlers)...}, std::forward<Variant>(v));
	}

	//! Visits the variants @p v1 and @p v2 using the handlers @p handlers.
	//! @note Credit to Nikolai Wuttke, "std::variant and the power of pattern matching".
	template <typename Variant1, typename Variant2, typename... Handlers>
	auto match2(Variant1&& v1, Variant2&& v2, Handlers&&... handlers) {
		return std::visit( //
			overloaded{std::forward<Handlers>(handlers)...},
			std::forward<Variant1>(v1),
			std::forward<Variant2>(v2));
	}
}
