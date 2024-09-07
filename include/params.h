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

#ifndef __PARAMS_H_INCLUDED__
#define __PARAMS_H_INCLUDED__

#include "type_mapping.h"
#include "types.h"
#include <stdexcept>

namespace easydbuspp {

template <typename V, std::size_t Index>
std::decay_t<V> extract_std_variant(GVariant* v);

template <typename T>
std::decay_t<T> from_gvariant(GVariant* v)
{
    using namespace std::string_literals;

    if constexpr (decay_same_v<T, int16_t>)
        return g_variant_get_int16(v);
    else if constexpr (decay_same_v<T, uint16_t>)
        return g_variant_get_uint16(v);
    else if constexpr (decay_same_v<T, int32_t>)
        return g_variant_get_int32(v);
    else if constexpr (decay_same_v<T, uint32_t>)
        return g_variant_get_uint32(v);
    else if constexpr (decay_same_v<T, int64_t>)
        return g_variant_get_int64(v);
    else if constexpr (decay_same_v<T, uint64_t>)
        return g_variant_get_uint64(v);
    else if constexpr (decay_same_v<T, double> || decay_same_v<T, float>)
        return g_variant_get_double(v);
    else if constexpr (decay_same_v<T, bool>)
        return g_variant_get_boolean(v);
    else if constexpr (decay_same_v<T, std::byte>)
        return std::byte {g_variant_get_byte(v)};
    else if constexpr (decay_same_v<T, std::string> || decay_same_v<T, object_path_t>)
        return g_variant_get_string(v, nullptr);
    else if constexpr (is_tuple_like_v<T>) {
        std::decay_t<T> ret;
        int             index = 0;

        auto extract_current_item = [v, &index](auto& arg) {
            g_variant_ptr child_value {g_variant_get_child_value(v, index++), g_variant_unref};

            if (!child_value)
                throw std::runtime_error {"nullptr child on GVariant -> std::tuple conversion!"};

            arg = from_gvariant<decltype(arg)>(child_value.get());
        };

        std::apply(
            [&extract_current_item](auto&&... args) {
                ((extract_current_item(args)), ...);
            },
            ret);

        return ret;
    } else if constexpr (is_vector_v<T>) {
        const std::string type {"a" + to_dbus_type_string<typename std::decay_t<T>::value_type>()};
        std::decay_t<T>   ret;

        GVariantIter* list {nullptr};
        g_variant_get(v, type.c_str(), &list);

        GVariant* rec {nullptr};

        while ((rec = g_variant_iter_next_value(list))) {
            ret.push_back(from_gvariant<typename std::decay_t<T>::value_type>(rec));
            g_variant_unref(rec);
        }

        g_variant_iter_free(list);

        return ret;
    } else if constexpr (is_map_like_v<T>) {
        using decayed_key_type    = typename std::decay_t<T>::key_type;
        using decayed_mapped_type = typename std::decay_t<T>::mapped_type;

        const std::string type {"a{" + to_dbus_type_string<decayed_key_type>()
                                + to_dbus_type_string<decayed_mapped_type>() + "}"};

        std::decay_t<T> ret;

        GVariantIter* list {nullptr};
        g_variant_get(v, type.c_str(), &list);

        GVariant* rec {nullptr};

        while ((rec = g_variant_iter_next_value(list))) {
            ret.insert(from_gvariant<std::pair<decayed_key_type, decayed_mapped_type>>(rec));
            g_variant_unref(rec);
        }

        g_variant_iter_free(list);

        return ret;
    } else if constexpr (is_variant_v<T>) {
        g_variant_ptr child_value {g_variant_get_child_value(v, 0), g_variant_unref};
        return extract_std_variant<T>(child_value.get());
    } else
        throw std::runtime_error {"Don't know how to extract "s + typeid(T).name() + " from a GVariant!"};
}

template <typename V, std::size_t Index = 0>
std::decay_t<V> extract_std_variant(GVariant* v)
{
    using variant_t = std::decay_t<V>;

    if constexpr (Index < std::variant_size_v<variant_t>) {
        if (g_variant_is_of_type(v, to_dbus_type<std::variant_alternative_t<Index, variant_t>>()))
            return from_gvariant<std::variant_alternative_t<Index, variant_t>>(v);

        return extract_std_variant<V, Index + 1>(v);
    }

    throw std::runtime_error {"No std::variant<> type matched the GVariant!"};
}

template <typename T>
std::decay_t<T> extract(GVariant* parameters, gsize index)
{
    if (!parameters)
        throw std::runtime_error {"nullptr parameters passed into extract<T>()!"};

    g_variant_ptr child_value {g_variant_get_child_value(parameters, index), g_variant_unref};

    return from_gvariant<T>(child_value.get());
}

template <typename T>
GVariant* to_gvariant(T t)
{
    using namespace std::string_literals;

    if constexpr (std::is_arithmetic_v<T> || decay_same_v<T, const char*>)
        return g_variant_new(to_dbus_type_string(t).c_str(), t);
    else if constexpr (decay_same_v<T, std::string>)
        return g_variant_new(to_dbus_type_string(t).c_str(), t.c_str());
    else if constexpr (decay_same_v<T, object_path_t>)
        return g_variant_new(to_dbus_type_string(t).c_str(), t.generic_string().c_str());
    else if constexpr (is_tuple_like_v<T>) {
        g_variant_builder_ptr builder {g_variant_builder_new(G_VARIANT_TYPE_TUPLE), g_variant_builder_unref};

        std::apply(
            [&builder](auto&&... args) {
                ((g_variant_builder_add_value(builder.get(), to_gvariant(args))), ...);
            },
            t);

        return g_variant_builder_end(builder.get());
    } else if constexpr (is_map_like_v<T>) {
        const std::string type {"a{" + to_dbus_type_string<typename T::key_type>()
                                + to_dbus_type_string<typename T::mapped_type>() + "}"};
        const std::string elem_type {"{@" + to_dbus_type_string<typename T::key_type>() + "@"
                                     + to_dbus_type_string<typename T::mapped_type>() + "}"};

        g_variant_builder_ptr builder {g_variant_builder_new(G_VARIANT_TYPE(type.c_str())), g_variant_builder_unref};

        for (auto&& [key, value] : t)
            g_variant_builder_add_value(builder.get(),
                                        g_variant_new(elem_type.c_str(), to_gvariant(key), to_gvariant(value)));

        return g_variant_builder_end(builder.get());
    } else if constexpr (is_variant_v<T>) {
        g_variant_builder_ptr builder {g_variant_builder_new(G_VARIANT_TYPE_VARIANT), g_variant_builder_unref};

        std::visit(
            [&builder](auto&& arg) {
                g_variant_builder_add_value(builder.get(), to_gvariant(arg));
            },
            t);

        return g_variant_builder_end(builder.get());
    } else if constexpr (is_vector_v<T>) {
        g_variant_builder_ptr builder {g_variant_builder_new(G_VARIANT_TYPE(to_dbus_type_string<T>().c_str())),
                                       g_variant_builder_unref};

        for (auto&& elem : t)
            g_variant_builder_add_value(builder.get(), to_gvariant(elem));

        return g_variant_builder_end(builder.get());
    } else
        throw std::runtime_error {"Don't know how to create a GVariant from "s + typeid(T).name() + "!"};
}

} // end of namespace easydbuspp

#endif // __PARAMS_H_INCLUDED__
