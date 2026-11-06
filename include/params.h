// SPDX-FileCopyrightText: © 2024 Răzvan Cojocaru <razvanc@mailbox.org>
//
// SPDX-License-Identifier: AGPL-3.0-only

#ifndef EASYDBUSPP_PARAMS_H_INCLUDED
#define EASYDBUSPP_PARAMS_H_INCLUDED

#include "type_mapping.h"
#include "types.h"
#include <gio/gunixfdlist.h>
#include <glib-unix.h>
#include <stdexcept>
#include <unistd.h>

namespace easydbuspp {

template <typename... Types>
void extract(GVariant* v, std::variant<Types...>& out);

template <typename T>
std::decay_t<T> from_gvariant(GVariant* v)
{
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

        GVariantIter* raw_list {nullptr};
        g_variant_get(v, type.c_str(), &raw_list);
        g_variant_iter_ptr list {raw_list, g_variant_iter_free};

        while (GVariant* raw_rec = g_variant_iter_next_value(list.get())) {
            g_variant_ptr rec {raw_rec, g_variant_unref};
            ret.push_back(from_gvariant<typename std::decay_t<T>::value_type>(rec.get()));
        }

        return ret;
    } else if constexpr (is_map_like_v<T>) {
        using decayed_key_type    = typename std::decay_t<T>::key_type;
        using decayed_mapped_type = typename std::decay_t<T>::mapped_type;

        const std::string type {"a{" + to_dbus_type_string<decayed_key_type>()
                                + to_dbus_type_string<decayed_mapped_type>() + "}"};

        std::decay_t<T> ret;

        GVariantIter* raw_list {nullptr};
        g_variant_get(v, type.c_str(), &raw_list);
        g_variant_iter_ptr list {raw_list, g_variant_iter_free};

        while (GVariant* raw_rec = g_variant_iter_next_value(list.get())) {
            g_variant_ptr rec {raw_rec, g_variant_unref};
            ret.insert(from_gvariant<std::pair<decayed_key_type, decayed_mapped_type>>(rec.get()));
        }

        return ret;
    } else if constexpr (is_variant_v<T>) {
        g_variant_ptr   child_value {g_variant_get_child_value(v, 0), g_variant_unref};
        std::decay_t<T> ret;
        extract(child_value.get(), ret);
        return ret;
    } else
        static_assert(always_false_v<T>, "Don't know how to extract T from a GVariant!");
}

template <typename... Types>
void extract(GVariant* v, std::variant<Types...>& out)
{
    bool matched {false};

    (
        [&] {
            if (!matched && g_variant_is_of_type(v, to_dbus_type<Types>())) {
                out     = from_gvariant<Types>(v);
                matched = true;
            }
        }(),
        ...);

    if (!matched)
        throw std::runtime_error {"GVariant type does not match any alternative in std::variant!"};
}

//! Like from_gvariant(), but throws if the GVariant isn't exactly of the D-Bus type that T maps to.
template <typename T>
std::decay_t<T> checked_from_gvariant(GVariant* v)
{
    if (!v)
        throw std::runtime_error {"nullptr GVariant passed into checked_from_gvariant<T>()!"};

    if (!g_variant_is_of_type(v, to_dbus_type<T>()))
        throw std::runtime_error {"Unexpected D-Bus type '" + std::string {g_variant_get_type_string(v)}
                                  + "', expected '" + to_dbus_type_string<T>() + "'!"};

    return from_gvariant<T>(v);
}

template <typename T>
std::decay_t<T> extract(GVariant* parameters, gsize index)
{
    if (!parameters)
        throw std::runtime_error {"nullptr parameters passed into extract<T>()!"};

    if (!g_variant_is_of_type(parameters, G_VARIANT_TYPE_TUPLE) || index >= g_variant_n_children(parameters))
        throw std::runtime_error {"No parameter at index " + std::to_string(index) + " (parameters are of type '"
                                  + g_variant_get_type_string(parameters) + "')!"};

    g_variant_ptr child_value {g_variant_get_child_value(parameters, index), g_variant_unref};

    return checked_from_gvariant<T>(child_value.get());
}

template <typename T>
GVariant* to_gvariant(const T& t)
{
    if constexpr (std::is_arithmetic_v<T> || decay_same_v<T, const char*>) {
        // Copy to a local so that a value (not a reference) is passed through C varargs.
        T copy = t;
        return g_variant_new(to_dbus_type_string(copy).c_str(), copy);
    } else if constexpr (decay_same_v<T, std::byte>)
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

        for (auto&& [key, value] : t) {
            // Held until g_variant_new() takes its own references, so the first isn't leaked if the second throws.
            g_variant_ptr key_variant {g_variant_ref_sink(to_gvariant(key)), g_variant_unref};
            g_variant_ptr value_variant {g_variant_ref_sink(to_gvariant(value)), g_variant_unref};

            g_variant_builder_add_value(builder.get(),
                                        g_variant_new(elem_type.c_str(), key_variant.get(), value_variant.get()));
        }

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
        static_assert(always_false_v<T>, "Don't know how to create a GVariant from T!");
}

//! Calls f() with every unix_fd_t nested anywhere inside value (tuples, pairs, vectors, map values, variants).
template <typename T, typename F>
void for_each_unix_fd(T&& value, F& f)
{
    using decayed_type = std::decay_t<T>;

    if constexpr (std::is_same_v<decayed_type, unix_fd_t>) {
        f(value);
    } else if constexpr (is_tuple_like_v<decayed_type>) {
        std::apply(
            [&f](auto&... elems) {
                (for_each_unix_fd(elems, f), ...);
            },
            value);
    } else if constexpr (is_vector_v<decayed_type>) {
        for (auto&& elem : value)
            for_each_unix_fd(elem, f);
    } else if constexpr (is_map_like_v<decayed_type>) {
        for (auto&& entry : value)
            for_each_unix_fd(entry.second, f);
    } else if constexpr (is_variant_v<decayed_type>) {
        std::visit(
            [&f](auto& alternative) {
                for_each_unix_fd(alternative, f);
            },
            value);
    }
}

//! Moves the fds out of input into the returned list, and replaces them in input with their list indexes.
template <typename... A>
g_unix_fd_list_ptr extract_g_unix_fd_list(std::tuple<A...>& input)
{
    g_unix_fd_list_ptr fd_list {nullptr, g_object_unref};

    auto append = [&fd_list](unix_fd_t& fd) {
        if (!fd_list)
            fd_list.reset(g_unix_fd_list_new());

        GError*     raw_error {nullptr};
        const gint  index = g_unix_fd_list_append(fd_list.get(), static_cast<gint32>(fd), &raw_error);
        g_error_ptr error {raw_error, g_error_free};

        if (error)
            throw std::runtime_error("Could not add UNIX fd: " + std::string {error->message});

        fd = unix_fd_t {index};
    };

    for_each_unix_fd(input, append);

    return fd_list;
}

//! Replaces the fd list indexes in inout with new fds that the caller owns (none are leaked on failure).
template <typename... A>
void set_up_from_g_unix_fd_list(GUnixFDList* fd_list, std::tuple<A...>& inout)
{
    std::vector<int> obtained;

    auto set_up = [&](unix_fd_t& fd) {
        if (!fd_list)
            throw std::runtime_error("UNIX fd parameter encountered but no fd list received!");

        // The index comes from the remote peer, and GLib only asserts (and returns -1) when it's out of range.
        if (static_cast<gint32>(fd) < 0 || static_cast<gint32>(fd) >= g_unix_fd_list_get_length(fd_list))
            throw std::runtime_error("UNIX fd index " + std::to_string(static_cast<gint32>(fd))
                                     + " is out of range of the received fd list!");

        GError*     raw_error {nullptr};
        const gint  new_fd = g_unix_fd_list_get(fd_list, static_cast<gint32>(fd), &raw_error);
        g_error_ptr error {raw_error, g_error_free};

        if (error)
            throw std::runtime_error("Could not extract UNIX fd: " + std::string {error->message});

        obtained.push_back(new_fd);
        fd = unix_fd_t {new_fd};
    };

    try {
        for_each_unix_fd(inout, set_up);
    } catch (...) {
        for (const int fd : obtained)
            close(fd);

        throw;
    }
}

} // end of namespace easydbuspp

#endif // EASYDBUSPP_PARAMS_H_INCLUDED
