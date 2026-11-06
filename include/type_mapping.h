// SPDX-FileCopyrightText: © 2024 Răzvan Cojocaru <razvanc@mailbox.org>
//
// SPDX-License-Identifier: AGPL-3.0-only

#ifndef EASYDBUSPP_TYPE_MAPPING_H_INCLUDED
#define EASYDBUSPP_TYPE_MAPPING_H_INCLUDED

#include "types.h"
#include <cstddef>
#include <cstdint>
#include <string>

namespace easydbuspp {

template <typename T>
std::string to_dbus_type_string()
{
    if constexpr (decay_same_v<T, int16_t>)
        return "n";
    else if constexpr (decay_same_v<T, uint16_t>)
        return "q";
    else if constexpr (decay_same_v<T, int32_t>)
        return "i";
    else if constexpr (decay_same_v<T, uint32_t>)
        return "u";
    else if constexpr (decay_same_v<T, int64_t>)
        return "x";
    else if constexpr (decay_same_v<T, uint64_t>)
        return "t";
    else if constexpr (decay_same_v<T, double> || decay_same_v<T, float>)
        return "d";
    else if constexpr (decay_same_v<T, std::byte>)
        return "y";
    else if constexpr (decay_same_v<T, unix_fd_t>)
        return "h";
    else if constexpr (decay_same_v<T, std::string> || decay_same_v<T, const char*>)
        return "s";
    else if constexpr (decay_same_v<T, object_path_t>)
        return "o";
    else if constexpr (is_variant_v<T>)
        return "v";
    else if constexpr (decay_same_v<T, bool>)
        return "b";
    else if constexpr (is_vector_v<T>)
        return "a" + to_dbus_type_string<typename std::decay_t<T>::value_type>();
    else if constexpr (is_tuple_like_v<T>) {
        std::decay_t<T> t;
        std::string     ret {"("};

        std::apply(
            [&ret](auto&&... args) {
                ((ret += to_dbus_type_string<decltype(args)>()), ...);
            },
            t);

        return ret + ")";
    } else if constexpr (is_map_like_v<T>)
        return "a{" + to_dbus_type_string<typename std::decay_t<T>::key_type>()
            + to_dbus_type_string<typename std::decay_t<T>::mapped_type>() + "}";
    else if constexpr (std::is_void_v<T>)
        return "()";
    else
        static_assert(always_false_v<T>, "Can't map T to a D-Bus type!");
}

template <typename T>
std::string to_dbus_type_string(T)
{
    return to_dbus_type_string<T>();
}

//! Returns the exact D-Bus type of T (e.g. "ai", not just "an array of anything").
template <typename T>
const GVariantType* to_dbus_type()
{
    // The GVariantType points into the string, so it has to live as long as the program does.
    static const std::string type_string {to_dbus_type_string<T>()};
    return G_VARIANT_TYPE(type_string.c_str());
}

template <typename T>
const GVariantType* to_dbus_type(T)
{
    return to_dbus_type<T>();
}

} // end of namespace easydbuspp

#endif // EASYDBUSPP_TYPE_MAPPING_H_INCLUDED
