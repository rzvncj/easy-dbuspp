// SPDX-FileCopyrightText: © 2024 Răzvan Cojocaru <razvanc@mailbox.org>
//
// SPDX-License-Identifier: AGPL-3.0-only

#ifndef EASYDBUSPP_TYPES_H_INCLUDED
#define EASYDBUSPP_TYPES_H_INCLUDED

#include <filesystem>
#include <gio/gio.h>
#include <map>
#include <memory>
#include <string>
#include <tuple>
#include <type_traits>
#include <unordered_map>
#include <variant>
#include <vector>

namespace easydbuspp {

//! The kinds of bus to connect to.
enum class bus_type_t { SESSION, SYSTEM };

using object_path_t = std::filesystem::path;

enum class unix_fd_t : gint32 {};

struct dbus_context {
    std::string   bus_name;
    std::string   interface_name;
    object_path_t object_path;
    std::string   name;
};

using g_variant_ptr         = std::unique_ptr<GVariant, decltype(&g_variant_unref)>;
using g_variant_builder_ptr = std::unique_ptr<GVariantBuilder, decltype(&g_variant_builder_unref)>;
using g_variant_iter_ptr    = std::unique_ptr<GVariantIter, decltype(&g_variant_iter_free)>;
using g_dbus_node_info_ptr  = std::unique_ptr<GDBusNodeInfo, decltype(&g_dbus_node_info_unref)>;
using g_unix_fd_list_ptr    = std::unique_ptr<GUnixFDList, decltype(&g_object_unref)>;
using g_error_ptr           = std::unique_ptr<GError, decltype(&g_error_free)>;

template <typename U, typename V>
constexpr bool decay_same_v = std::is_same_v<std::decay_t<U>, V>;

// Always-false helper for static_assert in if constexpr else branches.
// A plain static_assert(false) would be ill-formed at template definition
// time; making it dependent on T defers evaluation to instantiation.
template <typename>
inline constexpr bool always_false_v = false;

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

inline GBusType to_g_bus_type(easydbuspp::bus_type_t bus_type)
{
    switch (bus_type) {
    case easydbuspp::bus_type_t::SESSION:
        return G_BUS_TYPE_SESSION;
    case easydbuspp::bus_type_t::SYSTEM:
        return G_BUS_TYPE_SYSTEM;
    }

    throw std::runtime_error("Unknown bus type - this should never happen.");
}

} // end of namespace easydbuspp

#endif // EASYDBUSPP_TYPES_H_INCLUDED
