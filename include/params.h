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
#include <gio/gunixfdlist.h>
#include <glib-unix.h>
#include <stdexcept>

namespace easydbuspp {

template <typename... Types>
void extract(GVariant* v, std::variant<Types...>& out);

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
    else if constexpr (decay_same_v<T, unix_fd_t>)
        return unix_fd_t {g_variant_get_handle(v)};
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
        g_variant_ptr   child_value {g_variant_get_child_value(v, 0), g_variant_unref};
        std::decay_t<T> ret;
        extract(child_value.get(), ret);
        return ret;
    } else
        throw std::runtime_error {"Don't know how to extract "s + typeid(T).name() + " from a GVariant!"};
}

template <typename... Types>
void extract(GVariant* v, std::variant<Types...>& out)
{
    (
        [&]() {
            if (g_variant_is_of_type(v, to_dbus_type<Types>()))
                out = from_gvariant<Types>(v);
        }(),
        ...);
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
    else if constexpr (decay_same_v<T, std::byte>)
        return g_variant_new(to_dbus_type_string(t).c_str(), std::to_integer<uint8_t>(t));
    else if constexpr (decay_same_v<T, unix_fd_t>)
        return g_variant_new(to_dbus_type_string(t).c_str(), static_cast<gint32>(t));
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

template <typename... A>
GUnixFDList* extract_g_unix_fd_list(std::tuple<A...>& input)
{
    g_unix_fd_list_ptr fd_list {nullptr, g_object_unref};
    gint32             fd_list_index {0};

    auto extract = [&](auto& arg) {
        if constexpr (std::is_same_v<std::decay_t<decltype(arg)>, unix_fd_t>) {
            if (!fd_list)
                fd_list.reset(g_unix_fd_list_new());

            GError* error {nullptr};

            g_unix_fd_list_append(fd_list.get(), static_cast<gint32>(arg), &error);
            arg = unix_fd_t {fd_list_index++};

            if (error) {
                std::string error_message = error->message;
                g_error_free(error);

                throw std::runtime_error("Could not add UNIX fd: " + error_message);
            }
        }
    };

    std::apply(
        [&extract](auto&&... args) {
            ((extract(args)), ...);
        },
        input);

    return fd_list.release();
}

template <typename... A>
void set_up_from_g_unix_fd_list(GUnixFDList* fd_list, std::tuple<A...>& inout)
{
    auto set_up = [&](auto& arg) {
        if constexpr (std::is_same_v<std::decay_t<decltype(arg)>, unix_fd_t>) {
            if (!fd_list)
                throw std::runtime_error("UNIX fd parameter encountered but no fd list received!");

            GError* error {nullptr};
            arg = unix_fd_t {g_unix_fd_list_get(fd_list, static_cast<gint32>(arg), &error)};

            if (error) {
                std::string error_message = error->message;
                g_error_free(error);

                throw std::runtime_error("Could not extract UNIX fd: " + error_message);
            }
        }
    };

    std::apply(
        [&set_up](auto&&... args) {
            ((set_up(args)), ...);
        },
        inout);
}

} // end of namespace easydbuspp

#endif // __PARAMS_H_INCLUDED__
