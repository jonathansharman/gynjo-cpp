//! @file
//! @copyright See <a href="LICENSE.txt">LICENSE.txt</a>.

#pragma once

#include "tokens.hpp"
#include "visitation.hpp"

#include <boost/multiprecision/cpp_dec_float.hpp>

#include <memory>

namespace gynjo::val {
	//! Floating-point number.
	using num = boost::multiprecision::cpp_dec_float_100;

	//! Function.
	struct fun {
		//! Function parameter.
		struct param {
			std::string name;
			bool operator==(param const&) const = default;
		};
		std::vector<param> params;
		ast::ptr body;

		fun(std::vector<param> params, ast::ptr body) : params{params}, body{std::move(body)} {}

		fun(fun const& other) : params{other.params}, body{clone(other.body)} {}
		fun(fun&& other) = default;

		fun& operator=(fun const& other) {
			params = other.params;
			body = clone(other.body);
			return *this;
		}
		fun& operator=(fun&& other) = default;

		//! Because of the halting problem, this just does default, shallow equality checking on the body.
		bool operator==(fun const& other) const = default;
	};

	//! Union type of all Gynjo value types.
	using value = std::variant<num, struct tup, fun>;

	//! Unique pointer to a Gynjo value.
	using ptr = std::unique_ptr<value>;

	//! Convenience function for creating a Gynjo value pointer from a value @p val.
	template <typename T>
	auto make_value(T&& val) -> ptr;

	//! Tuple of Gynjo values.
	struct tup {
		std::vector<ptr> elems;

		template <typename... Args>
		tup(Args&&... args) {
			(elems.push_back(std::forward<Args>(args)), ...);
		}

		tup(tup const& other) {
			for (auto& elem : other.elems) {
				elems.push_back(make_value(*elem));
			}
		}
		tup(tup&& other) = default;

		tup& operator=(tup const& other) {
			elems.clear();
			for (auto& elem : other.elems) {
				elems.push_back(make_value(*elem));
			}
			return *this;
		}
		tup& operator=(tup&& other) = default;

		bool operator==(tup const& other) const {
			if (elems.size() != other.elems.size()) { return false; }
			for (std::size_t i = 0; i < elems.size(); ++i) {
				if (*elems[i] != *other.elems[i]) { return false; }
			}
			return true;
		}
	};

	template <typename T>
	auto make_value(T&& val) -> ptr {
		return std::make_unique<gynjo::val::value>(std::forward<T>(val));
	}

	//! Converts the value @p val to a user-readable string.
	inline auto to_string(value const& val) -> std::string {
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
			[](fun const& fun) { return "() -> " + to_string(fun.body); });
	}
}
