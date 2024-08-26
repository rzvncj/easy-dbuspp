#ifndef __PARAMS_H_INCLUDED__
#define __PARAMS_H_INCLUDED__

#include "type_mapping.h"
#include "types.h"
#include <gio/gio.h>
#include <memory>
#include <stdexcept>
#include <string>

template <typename T> std::decay_t<T> extract(GVariant* parameters, gsize index)
{
    using namespace std::string_literals;

    if (!parameters)
        throw std::runtime_error {"nullptr parameters passed into extract<T>()!"};

    std::unique_ptr<GVariant, decltype(&g_variant_unref)> child_value {g_variant_get_child_value(parameters, index),
                                                                       g_variant_unref};

    std::decay_t<T> ret {};

    if constexpr (decay_same_v<T, int16_t>)
        ret = g_variant_get_int16(child_value.get());
    else if constexpr (decay_same_v<T, uint16_t>)
        ret = g_variant_get_uint16(child_value.get());
    else if constexpr (decay_same_v<T, int32_t>)
        ret = g_variant_get_int32(child_value.get());
    else if constexpr (decay_same_v<T, uint32_t>)
        ret = g_variant_get_uint32(child_value.get());
    else if constexpr (decay_same_v<T, int64_t>)
        ret = g_variant_get_int64(child_value.get());
    else if constexpr (decay_same_v<T, uint64_t>)
        ret = g_variant_get_uint64(child_value.get());
    else if constexpr (decay_same_v<T, double> || decay_same_v<T, float>)
        ret = g_variant_get_double(child_value.get());
    else if constexpr (decay_same_v<T, bool>)
        ret = g_variant_get_boolean(child_value.get());
    else if constexpr (decay_same_v<T, std::string> || decay_same_v<T, object_path_t>)
        ret = g_variant_get_string(child_value.get(), nullptr);
    else
        throw std::runtime_error {"Don't know how to extract "s + typeid(T).name() + " from a GVariant!"};

    return ret;
}

template <typename T> GVariant* new_gvariant(T t)
{
    using namespace std::string_literals;

    // Integral or floating-point type.
    if constexpr (std::is_arithmetic_v<T>)
        return g_variant_new(to_dbus_type(t).c_str(), t);
    else if constexpr (decay_same_v<T, std::string>)
        return g_variant_new(to_dbus_type(t).c_str(), t.c_str());
    else if constexpr (decay_same_v<T, const char*>)
        return g_variant_new(to_dbus_type(t).c_str(), t);
    else
        throw std::runtime_error {"Don't know how to create a GVariant from "s + typeid(T).name() + "!"};
}

template <typename... Args> GVariant* to_gvariant(const std::tuple<Args...>& t)
{
    std::unique_ptr<GVariantBuilder, decltype(&g_variant_builder_unref)> builder {
        g_variant_builder_new(G_VARIANT_TYPE_TUPLE), g_variant_builder_unref};

    std::apply(
        [&builder](auto&&... args) {
            ((g_variant_builder_add_value(builder.get(), new_gvariant(args))), ...);
        },
        t);

    return g_variant_builder_end(builder.get());
}

#endif // __PARAMS_H_INCLUDED__
