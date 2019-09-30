//! @file
//! @copyright See <a href="LICENSE.txt">LICENSE.txt</a>.

#pragma once

#include "ast.hpp"
#include "tokens.hpp"
#include "visitation.hpp"

#include <boost/multiprecision/cpp_dec_float.hpp>

#include <memory>

namespace gynjo {
	struct environment;

	namespace val {
		//! Floating-point number.
		using num = boost::multiprecision::cpp_dec_float_100;

		//! Union type of all Gynjo value types.
		using value = std::variant<num, struct tup, struct closure>;

		//! A function along with the environment in which it was evaluated.
		struct closure {
			//! Function parameter.
			struct param {
				std::string name;
				bool operator==(param const&) const = default;
			};

			std::vector<param> params;
			ast::ptr body;
			std::unique_ptr<environment> env;

			closure(std::vector<param> params, ast::ptr body, std::unique_ptr<environment> env);

			closure(closure const& that);
			closure(closure&&) = default;

			~closure();

			closure& operator=(closure const& that);
			closure& operator=(closure&&) = default;

			//! Because of the halting problem, this just does shallow equality checking on the body.
			bool operator==(closure const& other) const = default;
		};

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
				(elems.push_back(std::move(args)), ...);
			}

			tup(tup const& that);
			tup(tup&&) = default;

			tup& operator=(tup const& that);
			tup& operator=(tup&&) = default;

			bool operator==(tup const& that) const;
		};

		template <typename T>
		auto make_value(T&& val) -> ptr {
			return std::make_unique<gynjo::val::value>(std::forward<T>(val));
		}

		//! Converts the value @p val to a user-readable string.
		auto to_string(value const& val) -> std::string;
	}
}
