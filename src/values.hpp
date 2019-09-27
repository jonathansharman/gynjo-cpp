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
	};

	//! Union type of all Gynjo value types.
	using val = std::variant<num, struct tup, fun>;

	//! Unique pointer to a Gynjo value.
	using ptr = std::unique_ptr<val>;

	//! Convenience function for creating a Gynjo value pointer from a value @p val.
	template <typename T>
	auto make_val(T&& val) -> ptr;

	//! Tuple of Gynjo values.
	struct tup {
		std::vector<ptr> elements;

		tup(std::vector<ptr> elements) : elements{std::move(elements)} {}

		tup(tup const& other) {
			for (auto& element : other.elements) {
				elements.push_back(make_val(*element));
			}
		}
		tup(tup&& other) = default;

		tup& operator=(tup const& other) {
			elements.clear();
			for (auto& element : other.elements) {
				elements.push_back(make_val(*element));
			}
			return *this;
		}
		tup& operator=(tup&& other) = default;
	};

	template <typename T>
	auto make_val(T&& val) -> ptr {
		return std::make_unique<gynjo::val::val>(std::forward<T>(val));
	}

	//! Converts the value @p val to a user-readable string.
	inline auto to_string(val const& val) -> std::string {
		using namespace std::string_literals;
		return match(
			val,
			[](num const& num) { return num.str(); },
			[](tup const& tup) {
				std::string result = "(";
				if (!tup.elements.empty()) { result += to_string(*tup.elements.front()); }
				for (auto it = tup.elements.begin() + 1; it != tup.elements.end(); ++it) {
					result += ", " + to_string(**it);
				}
				result += ")";
				return result;
			},
			[](fun const& fun) { return "() -> " + to_string(fun.body); });
	}
}
