//! @file
//! @copyright See <a href="LICENSE.txt">LICENSE.txt</a>.

#include "values.hpp"

#include "environment.hpp"

namespace gynjo::val {
	closure::~closure() = default;

	tup::tup() : elems{std::make_shared<std::vector<value>>()} {}

	tup::tup(std::shared_ptr<std::vector<value>> elems) : elems{std::move(elems)} {}

	auto tup::operator==(tup const& that) const noexcept -> bool {
		return *elems == *that.elems;
	}

	auto list::operator==(list const& that) const noexcept -> bool {
		return *head == *that.head && *tail == *that.tail;
	}

	auto to_string(value const& val, std::shared_ptr<environment> const& env) -> std::string {
		using namespace std::string_literals;
		return match(
			val,
			[](tok::boolean const& b) { return tok::to_string(b); },
			[&](num const& num) {
				constexpr auto default_precision = 12;
				auto const o_precision = env->lookup("precision");
				// Use default precision if there's no precision variable or if it's not an integer.
				auto const precision = o_precision ? as_int(*o_precision).value_or(default_precision) : default_precision;
				return num.str(precision);
			},
			[](std::string const& str) { return tok::to_string(str); },
			[&](tup const& tup) {
				std::string result = "(";
				if (!tup.elems->empty()) {
					result += to_string(tup.elems->front(), env);
					for (auto it = tup.elems->begin() + 1; it != tup.elems->end(); ++it) {
						result += ", " + to_string(*it, env);
					}
				}
				result += ")";
				return result;
			},
			[](empty) { return "[]"s; },
			[&](list const& list) {
				std::string result = "[" + to_string(*list.head, env);
				auto current = list.tail;
				while (!std::holds_alternative<empty>(*current)) {
					auto const& current_list = std::get<val::list>(*current);
					result += ", " + to_string(*current_list.head, env);
					current = current_list.tail;
				}
				result += "]";
				return result;
			},
			[&](closure const& c) { return to_string(expr{c.f}); });
	}

	auto as_int(value const& val) -> std::optional<int> {
		return std::holds_alternative<num>(val) ? std::make_optional(std::get<num>(val).convert_to<int>()) : std::nullopt;
	}
}
