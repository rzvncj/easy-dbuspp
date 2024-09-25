// SPDX-FileCopyrightText: © 2024 Răzvan Cojocaru <rzvncj@gmail.com>
//
// SPDX-License-Identifier: AGPL-3.0-only

#ifndef __PROXY_INL_INCLUDED__
#define __PROXY_INL_INCLUDED__

namespace easydbuspp {

template <typename R, typename... A>
R proxy::call(const std::string& method_name, A... parameters) const
{
    std::tuple<std::decay_t<A>...> fn_args {parameters...};
    GError*                        error {nullptr};
    GUnixFDList*                   out_fd_list {nullptr};

    g_unix_fd_list_ptr fd_list {extract_g_unix_fd_list(fn_args), g_object_unref};

    g_variant_ptr result {g_dbus_proxy_call_with_unix_fd_list_sync(proxy_, method_name.c_str(), to_gvariant(fn_args),
                                                                   G_DBUS_CALL_FLAGS_NONE, -1, fd_list.get(),
                                                                   &out_fd_list, nullptr, &error),
                          g_variant_unref};

    g_unix_fd_list_ptr out_fd_list_raii_holder {out_fd_list, g_object_unref};

    if (!result) {
        std::string error_message = error->message;
        g_error_free(error);

        throw std::runtime_error("Proxy method call error: " + error_message);
    }

    if constexpr (!std::is_void_v<R>) {
        if constexpr (is_tuple_like_v<R>) {
            auto ret = from_gvariant<R>(result.get());
            set_up_from_g_unix_fd_list(out_fd_list, ret);
            return ret;
        } else {
            auto ret = from_gvariant<std::tuple<R>>(result.get());
            set_up_from_g_unix_fd_list(out_fd_list, ret);
            return std::get<0>(ret);
        }
    }
}

template <typename T>
T proxy::cached_property(const std::string& property_name) const
{
    g_variant_ptr result {g_dbus_proxy_get_cached_property(proxy_, property_name.c_str()), g_variant_unref};

    if (!result)
        throw std::runtime_error("Property '" + property_name
                                 + "' could not be read (doesn't exist or is write-only)!");

    return from_gvariant<T>(result.get());
}

template <typename T>
void proxy::cached_property(const std::string& property_name, const T& new_value)
{
    g_dbus_proxy_set_cached_property(proxy_, property_name.c_str(), to_gvariant(new_value));
}

template <typename T>
void proxy::property(const std::string& property_name, const T& new_value)
{
    proxy props_proxy {session_manager_, bus_name_, "org.freedesktop.DBus.Properties", object_path_};

    if constexpr (decay_same_v<T, char*>)
        props_proxy.call<void>("Set", interface_name_, property_name, std::variant<std::string> {new_value});
    else
        props_proxy.call<void>("Set", interface_name_, property_name, std::variant<T> {new_value});
}

template <typename T>
T proxy::property(const std::string& property_name) const
{
    proxy props_proxy {session_manager_, bus_name_, "org.freedesktop.DBus.Properties", object_path_};
    return std::get<0>(props_proxy.call<std::variant<T>>("Get", interface_name_, property_name));
}

} // end of namespace easydbuspp

#endif // __PROXY_INL_INCLUDED__
