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

#ifndef __TYPE_MAPPING_H_INCLUDED__
#define __TYPE_MAPPING_H_INCLUDED__

#include "types.h"
#include <cstddef>
#include <cstdint>
#include <stdexcept>
#include <string>
#include <variant>

namespace easydbuspp {

template <typename T>
std::string to_dbus_type_string()
{
    using namespace std::string_literals;

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
        throw std::runtime_error {"Can't map "s + typeid(T).name() + " to a D-Bus type!"};
}

template <typename T>
std::string to_dbus_type_string(T)
{
    return to_dbus_type_string<T>();
}

template <typename T>
const GVariantType* to_dbus_type()
{
    using namespace std::string_literals;

    if constexpr (decay_same_v<T, int16_t>)
        return G_VARIANT_TYPE_INT16;
    else if constexpr (decay_same_v<T, uint16_t>)
        return G_VARIANT_TYPE_UINT16;
    else if constexpr (decay_same_v<T, int32_t>)
        return G_VARIANT_TYPE_INT32;
    else if constexpr (decay_same_v<T, uint32_t>)
        return G_VARIANT_TYPE_UINT32;
    else if constexpr (decay_same_v<T, int64_t>)
        return G_VARIANT_TYPE_INT64;
    else if constexpr (decay_same_v<T, uint64_t>)
        return G_VARIANT_TYPE_UINT64;
    else if constexpr (decay_same_v<T, double> || decay_same_v<T, float>)
        return G_VARIANT_TYPE_DOUBLE;
    else if constexpr (decay_same_v<T, std::byte>)
        return G_VARIANT_TYPE_BYTE;
    else if constexpr (decay_same_v<T, std::string>)
        return G_VARIANT_TYPE_STRING;
    else if constexpr (decay_same_v<T, object_path_t>)
        return G_VARIANT_TYPE_OBJECT_PATH;
    else if constexpr (is_variant_v<T>)
        return G_VARIANT_TYPE_VARIANT;
    else if constexpr (decay_same_v<T, bool>)
        return G_VARIANT_TYPE_BOOLEAN;
    else if constexpr (is_vector_v<T>)
        return G_VARIANT_TYPE_ARRAY;
    else if constexpr (is_tuple_like_v<T>)
        return G_VARIANT_TYPE_TUPLE;
    else if constexpr (is_map_like_v<T>)
        return G_VARIANT_TYPE_DICTIONARY;
    else if constexpr (std::is_void_v<T>)
        return G_VARIANT_TYPE_UNIT;
    else
        throw std::runtime_error {"Can't map "s + typeid(T).name() + " to a D-Bus type!"};
}

template <typename T>
const GVariantType* to_dbus_type(T)
{
    return to_dbus_type<T>();
}

} // end of namespace easydbuspp

#endif // __TYPE_MAPPING_H_INCLUDED__
