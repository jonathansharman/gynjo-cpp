//! @file
//! @copyright See <a href="LICENSE.txt">LICENSE.txt</a>.

#include "values.hpp"

#include "environment.hpp"

namespace gynjo::val {
	closure::closure(std::vector<param> params, ast::ptr body, std::unique_ptr<environment> env)
		: params{std::move(params)}, body{std::move(body)}, env{std::move(env)} {}

	closure::closure(closure const& that)
		: params{that.params}, body{clone(*that.body)}, env{std::make_unique<environment>(*that.env)} {}

	closure::~closure() = default;

	closure& closure::operator=(closure const& that) {
		params = that.params;
		body = clone(*that.body);
		env = std::make_unique<environment>(*that.env);
		return *this;
	}

	tup::tup(tup const& that) {
		for (auto& elem : that.elems) {
			elems.push_back(make_value(*elem));
		}
	}

	tup& tup::operator=(tup const& that) {
		elems.clear();
		for (auto& elem : that.elems) {
			elems.push_back(make_value(*elem));
		}
		return *this;
	}

	bool tup::operator==(tup const& that) const {
		if (elems.size() != that.elems.size()) { return false; }
		for (std::size_t i = 0; i < elems.size(); ++i) {
			if (*elems[i] != *that.elems[i]) { return false; }
		}
		return true;
	}

	auto to_string(value const& val) -> std::string {
		using namespace std::string_literals;
		return match(
			val,
			[](num const& num) { return num.str(); },
			[](tup const& tup) {
				std::string result = "(";
				if (!tup.elems.empty()) {
					result += to_string(*tup.elems.front());
					for (auto it = tup.elems.begin() + 1; it != tup.elems.end(); ++it) {
						result += ", " + to_string(**it);
					}
				}
				result += ")";
				return result;
			},
			[](closure const& f) {
				std::string result = "(";
				if (!f.params.empty()) {
					result += f.params.front().name;
					for (auto it = f.params.begin() + 1; it != f.params.end(); ++it) {
						result += ", " + it->name;
					}
				}
				result += ") -> " + ast::to_string(*f.body);
				return result;
			});
	}
}
