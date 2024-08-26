#ifndef __TYPE_MAPPING_H_INCLUDED__
#define __TYPE_MAPPING_H_INCLUDED__

#include <any>
#include <cstddef>
#include <cstdint>
#include <stdexcept>
#include <string>

#include "types.h"

template <typename T> std::string to_dbus_type()
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
    else if constexpr (decay_same_v<T, std::string> || decay_same_v<T, const char*>)
        return "s";
    else if constexpr (decay_same_v<T, object_path_t>)
        return "o";
    else if constexpr (decay_same_v<T, std::any>)
        return "v";
    else if constexpr (decay_same_v<T, bool>)
        return "b";
    else if constexpr (is_vector_v<T>)
        return "a" + to_dbus_type<typename std::decay_t<T>::value_type>();
    else if constexpr (is_tuple_v<T>) {
        std::decay_t<T> t;
        std::string     ret {"("};

        std::apply(
            [&ret](auto&&... args) {
                ((ret += to_dbus_type<decltype(args)>()), ...);
            },
            t);

        return ret + ")";
    } else if constexpr (is_map_v<T>)
        return "{" + to_dbus_type<typename std::decay_t<T>::key_type>()
            + to_dbus_type<typename std::decay_t<T>::mapped_type>() + "}";
    else if constexpr (std::is_void_v<T>)
        return "()";
    else
        throw std::runtime_error {"Can't map "s + typeid(T).name() + " to a D-Bus type!"};
}

template <typename T> std::string to_dbus_type(T)
{
    return to_dbus_type<T>();
}

#endif // __TYPE_MAPPING_H_INCLUDED__
