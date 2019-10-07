//! @file
//! @copyright See <a href="LICENSE.txt">LICENSE.txt</a>.

#include "values.hpp"

#include "environment.hpp"

namespace gynjo::val {
	closure::~closure() = default;

	tup::tup() : elems{std::make_shared<std::vector<value>>()} {}

	tup::tup(std::shared_ptr<std::vector<value>> elems) : elems{std::move(elems)} {}

	bool tup::operator==(tup const& that) const noexcept {
		return *elems == *that.elems;
	}

	bool list::operator==(list const& that) const noexcept {
		return *head == *that.head && *tail == *that.tail;
	}

	auto to_string(value const& val) -> std::string {
		using namespace std::string_literals;
		return match(
			val,
			[](tok::boolean const& b) { return tok::to_string(b); },
			[](num const& num) { return num.str(); },
			[](tup const& tup) {
				std::string result = "(";
				if (!tup.elems->empty()) {
					result += to_string(tup.elems->front());
					for (auto it = tup.elems->begin() + 1; it != tup.elems->end(); ++it) {
						result += ", " + to_string(*it);
					}
				}
				result += ")";
				return result;
			},
			[](empty) { return "[]"s; },
			[](list const& list) {
				std::string result = "[" + to_string(*list.head);
				auto current = list.tail;
				while (!std::holds_alternative<empty>(*current)) {
					auto const& current_list = std::get<val::list>(*current);
					result += ", " + to_string(*current_list.head);
					current = current_list.tail;
				}
				result += "]";
				return result;
			},
			[](closure const& c) { return ast::to_string(c.f); });
	}
}
