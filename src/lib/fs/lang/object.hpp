#pragma once

#include <fs/utility/type_traits.hpp>
#include <fs/utility/better_enum.hpp>
#include <fs/lang/position_tag.hpp>
#include <fs/lang/primitive_types.hpp>
#include <fs/lang/action_set.hpp>
#include <fs/lang/queries.hpp>

#include <boost/container/small_vector.hpp>

#include <variant>

namespace fs::lang
{

using object_variant = std::variant<
	none,
	temp,
	boolean,
	floating_point,
	integer,
	socket_group,
	influence,
	rarity,
	shape,
	suit,
	string,
	action_set
>;

BETTER_ENUM(object_type, int,
	none,
	temp,
	boolean,
	floating_point,
	integer,
	socket_group,
	influence,
	rarity,
	shape,
	suit,
	string,
	action_set)


[[nodiscard]]
object_type type_of_object(const object_variant& obj) noexcept;

struct single_object
{
	object_type type() const noexcept
	{
		return type_of_object(value);
	}

	object_variant value;
	position_tag origin;
};

inline bool operator==(const single_object& lhs, const single_object& rhs)
{
	return lhs.value == rhs.value;
}

inline bool operator!=(const single_object& lhs, const single_object& rhs)
{
	return !(lhs == rhs);
}

[[nodiscard]] inline
std::string_view to_string_view(object_type type) noexcept
{
	return type._to_string();
}

struct object
{
	// Use small_vector because most lang sequences will have very limited number of elements -
	// the longest sequences are color (R, G, B, + optonal A) and arrays of strings for string-based
	// conditions like BaseType.
	// This container optimizes storage so that only long arrays of strings will allocate.
	using container_type = boost::container::small_vector<single_object, 4>;

	container_type values;
	position_tag origin;
};

bool operator==(const object& lhs, const object& rhs) noexcept;

inline
bool operator!=(const object& lhs, const object& rhs) noexcept { return !(lhs == rhs); }

namespace detail {
	template <typename T> [[nodiscard]] constexpr
	object_type type_to_enum_impl() noexcept
	{
		static_assert(sizeof(T) == 0, "missing implementation for this type");
		return object_type::none; // return statement only to silence compiler/editors
	}

	template <> constexpr
	object_type type_to_enum_impl<none>() noexcept { return object_type::none; }
	template <> constexpr
	object_type type_to_enum_impl<temp>() noexcept { return object_type::temp; }
	template <> constexpr
	object_type type_to_enum_impl<boolean>() noexcept { return object_type::boolean; }
	template <> constexpr
	object_type type_to_enum_impl<floating_point>() noexcept { return object_type::floating_point; }
	template <> constexpr
	object_type type_to_enum_impl<integer>() noexcept { return object_type::integer; }
	template <> constexpr
	object_type type_to_enum_impl<socket_group>() noexcept { return object_type::socket_group; }
	template <> constexpr
	object_type type_to_enum_impl<influence>() noexcept { return object_type::influence; }
	template <> constexpr
	object_type type_to_enum_impl<rarity>() noexcept { return object_type::rarity; }
	template <> constexpr
	object_type type_to_enum_impl<shape>() noexcept { return object_type::shape; }
	template <> constexpr
	object_type type_to_enum_impl<suit>() noexcept { return object_type::suit; }
	template <> constexpr
	object_type type_to_enum_impl<string>() noexcept { return object_type::string; }
	template <> constexpr
	object_type type_to_enum_impl<action_set>() noexcept { return object_type::action_set; }
}

// TODO change to _v trait
template <typename T> [[nodiscard]] constexpr
object_type type_to_enum() noexcept
{
	static_assert(
		traits::is_variant_alternative_v<T, object_variant>,
		"T must be one of object type alternatives");

	return detail::type_to_enum_impl<T>();
}

}
