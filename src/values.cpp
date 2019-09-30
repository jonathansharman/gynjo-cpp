//! @file
//! @copyright See <a href="LICENSE.txt">LICENSE.txt</a>.

#include "values.hpp"

#include "environment.hpp"

namespace gynjo::val {
	closure::closure(ast::lambda lambda, std::unique_ptr<environment> env)
		: lambda{std::move(lambda)}, env{std::move(env)} {}

	closure::closure(closure const& that) : lambda{that.lambda}, env{std::make_unique<environment>(*that.env)} {}

	closure::~closure() = default;

	closure& closure::operator=(closure const& that) {
		lambda = that.lambda;
		env = std::make_unique<environment>(*that.env);
		return *this;
	}

	tup::tup() : elems{std::make_unique<std::vector<value>>()} {}

	tup::tup(std::unique_ptr<std::vector<value>> elems) : elems{std::move(elems)} {}

	tup::tup(tup const& that) {
		elems = std::make_unique<std::vector<value>>(*that.elems);
	}

	tup& tup::operator=(tup const& that) {
		elems = std::make_unique<std::vector<value>>(*that.elems);
		return *this;
	}

	bool tup::operator==(tup const& that) const {
		return *elems == *that.elems;
	}

	auto to_string(value const& val) -> std::string {
		using namespace std::string_literals;
		return match(
			val,
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
			[](closure const& f) { return ast::to_string(f.lambda); });
	}
}
