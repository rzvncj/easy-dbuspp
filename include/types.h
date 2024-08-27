// easy-dbuspp
// Copyright (C) 2024-  Răzvan Cojocaru <rzvncj@gmail.com>
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU Affero General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU Affero General Public License for more details.
//
// You should have received a copy of the GNU Affero General Public License
// along with this program.  If not, see <http://www.gnu.org/licenses/>.

#ifndef __TYPES_H_INCLUDED__
#define __TYPES_H_INCLUDED__

#include <filesystem>
#include <gio/gio.h>
#include <map>
#include <memory>
#include <tuple>
#include <type_traits>
#include <unordered_map>
#include <variant>
#include <vector>

namespace easydbuspp {

using object_path_t = std::filesystem::path;

using g_variant_ptr         = std::unique_ptr<GVariant, decltype(&g_variant_unref)>;
using g_variant_builder_ptr = std::unique_ptr<GVariantBuilder, decltype(&g_variant_builder_unref)>;
using g_dbus_node_info_ptr  = std::unique_ptr<GDBusNodeInfo, decltype(&g_dbus_node_info_unref)>;

template <typename U, typename V>
constexpr bool decay_same_v = std::is_same_v<std::decay_t<U>, V>;

template <typename T>
struct is_output_type {
    static constexpr bool value
        = std::is_lvalue_reference_v<T> && !std::is_const_v<typename std::remove_reference_t<T>>;
};

template <typename T>
inline constexpr bool is_output_type_v = is_output_type<T>::value;

// is_specialization_of<> lifted from the WG21 P2098R0 proposal document.
template <typename T, template <typename...> typename Primary>
struct is_specialization_of : std::false_type {};

template <template <typename...> typename Primary, typename... Args>
struct is_specialization_of<Primary<Args...>, Primary> : std::true_type {};

template <typename T, template <typename...> typename Primary>
inline constexpr bool is_specialization_of_v = is_specialization_of<T, Primary>::value;

template <typename T>
inline constexpr bool is_tuple_v = is_specialization_of_v<std::decay_t<T>, std::tuple>;

template <typename T>
inline constexpr bool is_pair_v = is_specialization_of_v<std::decay_t<T>, std::pair>;

template <typename T>
inline constexpr bool is_vector_v = is_specialization_of_v<std::decay_t<T>, std::vector>;

template <typename T>
inline constexpr bool is_map_v = is_specialization_of_v<std::decay_t<T>, std::map>;

template <typename T>
inline constexpr bool is_variant_v = is_specialization_of_v<std::decay_t<T>, std::variant>;

template <typename T>
inline constexpr bool is_unordered_map_v = is_specialization_of_v<std::decay_t<T>, std::unordered_map>;

template <typename T>
inline constexpr bool is_tuple_like_v = is_tuple_v<T> || is_pair_v<T>;

template <typename T>
inline constexpr bool is_map_like_v = is_map_v<T> || is_unordered_map_v<T>;

} // end of namespace easydbuspp

#endif // __TYPES_H_INCLUDED__
