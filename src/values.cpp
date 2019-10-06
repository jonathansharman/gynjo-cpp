//! @file
//! @copyright See <a href="LICENSE.txt">LICENSE.txt</a>.

#include "values.hpp"

#include "environment.hpp"

namespace gynjo::val {
	closure::~closure() = default;

	list::list() : elems{std::make_shared<std::vector<value>>()} {}

	list::list(std::shared_ptr<std::vector<value>> elems) : elems{std::move(elems)} {}

	bool list::operator==(list const& that) const {
		return *elems == *that.elems;
	}

	auto to_string(value const& val) -> std::string {
		using namespace std::string_literals;
		return match(
			val,
			[](tok::boolean const& b) { return tok::to_string(b); },
			[](num const& num) { return num.str(); },
			[](list const& list) {
				std::string result = "(";
				if (!list.elems->empty()) {
					result += to_string(list.elems->front());
					for (auto it = list.elems->begin() + 1; it != list.elems->end(); ++it) {
						result += ", " + to_string(*it);
					}
				}
				result += ")";
				return result;
			},
			[](closure const& c) { return ast::to_string(c.f); });
	}
}
