//! @file
//! @copyright See <a href="LICENSE.txt">LICENSE.txt</a>.

#pragma once

#include "ast.hpp"
#include "environment_fwd.hpp"
#include "tokens.hpp"
#include "visitation.hpp"

#include <boost/multiprecision/cpp_dec_float.hpp>

#include <memory>

namespace gynjo {
	namespace val {
		//! Floating-point number.
		using num = boost::multiprecision::cpp_dec_float_100;

		//! Union type of all Gynjo value types.
		using value = std::variant<tok::boolean, num, struct tup, struct list, struct closure>;

		//! A lambda along with the environment in which it was called.
		struct closure {
			ast::lambda f;
			std::shared_ptr<environment> env;

			~closure();

			//! Because of the halting problem, this just does shallow equality checking on lambda bodies.
			bool operator==(closure const& other) const noexcept = default;
		};

		//! Tuple of Gynjo values.
		struct tup {
			std::shared_ptr<std::vector<value>> elems;

			tup();
			explicit tup(std::shared_ptr<std::vector<value>> elems);

			bool operator==(tup const& that) const;
		};

		template <typename... Args>
		auto make_tup(Args&&... args) {
			auto elems = std::make_shared<std::vector<value>>();
			(elems->push_back(std::move(args)), ...);
			return tup{std::move(elems)};
		}

		//! List of Gynjo values.
		struct list {
			std::shared_ptr<std::vector<value>> elems;

			list();
			explicit list(std::shared_ptr<std::vector<value>> elems);

			bool operator==(list const& that) const;
		};

		template <typename... Args>
		auto make_list(Args&&... args) {
			auto elems = std::make_shared<std::vector<value>>();
			(elems->push_back(std::move(args)), ...);
			return list{std::move(elems)};
		}

		//! Converts the value @p val to a user-readable string.
		auto to_string(value const& val) -> std::string;
	}
}
