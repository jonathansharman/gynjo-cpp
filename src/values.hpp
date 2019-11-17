//! @file
//! @copyright See <a href="LICENSE.txt">LICENSE.txt</a>.

#pragma once

#include "environment_fwd.hpp"
#include "expr.hpp"
#include "tokens.hpp"
#include "visitation.hpp"

#include <boost/multiprecision/cpp_dec_float.hpp>

#include <memory>

namespace gynjo {
	namespace val {
		//! Floating-point number.
		using num = boost::multiprecision::cpp_dec_float_100;

		//! Union type of all Gynjo value types.
		using value = std::variant<tok::boolean, num, std::string, struct tup, struct empty, struct list, struct closure>;

		//! Shared pointer to a Gynjo value.
		using ptr = std::shared_ptr<value>;

		//! A lambda along with the environment in which it was called.
		struct closure {
			lambda f;
			std::shared_ptr<environment> env;

			~closure();

			//! Because of the halting problem, this just does shallow equality checking on lambda bodies.
			auto operator==(closure const&) const noexcept -> bool = default;
		};

		//! Tuple of Gynjo values.
		struct tup {
			std::shared_ptr<std::vector<value>> elems;

			tup();
			explicit tup(std::shared_ptr<std::vector<value>> elems);

			auto operator==(tup const&) const noexcept -> bool;
		};

		template <typename... Args>
		auto make_tup(Args&&... args) {
			auto elems = std::make_shared<std::vector<value>>();
			(elems->push_back(std::move(args)), ...);
			return tup{std::move(elems)};
		}

		//! The empty Gynjo type.
		struct empty {
			auto operator==(empty const&) const noexcept -> bool = default;
		};

		//! Functional list of Gynjo values.
		struct list {
			//! The top value of this list.
			ptr head;
			//! Either another list or empty.
			ptr tail;

			list(ptr head, ptr tail);

			list(list const&) = default;
			list(list&&) noexcept = default;

			~list() noexcept;

			list& operator=(list const&) = default;
			list& operator=(list&&) noexcept = default;

			auto operator==(list const&) const noexcept -> bool;
		};

		//! Convenience function for creating a value pointer from @p value.
		template <typename T>
		auto make_value(T&& value) {
			return std::make_shared<val::value>(std::forward<T>(value));
		}

		template <typename... Args>
		auto make_list(Args&&... args) {
			val::value result = val::empty{};
			((result = val::list{make_value(std::move(args)), make_value(std::move(result))}), ...);
			return result;
		}

		//! Converts the value @p val to a user-readable string.
		auto to_string(value const& val) -> std::string;
	}
}
