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

	list::list(ptr head, ptr tail) : head{std::move(head)}, tail{std::move(tail)} {}

	list::~list() noexcept {
		// To avoid stack overflow, destroy iteratively by eating the tail.
		auto tail_eater = std::move(tail);
		while (tail_eater != nullptr && std::holds_alternative<val::list>(*tail_eater)) {
			tail_eater = std::move(std::get<val::list>(*tail_eater).tail);
		}
	}

	auto list::operator==(list const& that) const noexcept -> bool {
		return *head == *that.head && *tail == *that.tail;
	}

	auto to_string(value const& val) -> std::string {
		using namespace std::string_literals;
		return match(
			val,
			[](tok::boolean const& b) { return tok::to_string(b); },
			[](num const& num) { return num.str(); },
			[](std::string const& str) { return tok::to_string(str); },
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
			[](closure const& c) { return to_string(expr{c.f}); });
	}
}
