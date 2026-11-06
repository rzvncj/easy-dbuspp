// SPDX-FileCopyrightText: © 2024 Răzvan Cojocaru <razvanc@mailbox.org>
//
// SPDX-License-Identifier: AGPL-3.0-only

#ifndef EASYDBUSPP_PROXY_INL_INCLUDED
#define EASYDBUSPP_PROXY_INL_INCLUDED

namespace easydbuspp {

template <typename R, typename... A>
R proxy::call(const std::string& method_name, const A&... parameters) const
{
    // GLib rejects invalid names without setting an error, so catch them first.
    if (!g_dbus_is_member_name(method_name.c_str()))
        throw std::runtime_error("Proxy method call error: '" + method_name + "' is not a valid D-Bus method name");

    auto         fn_args = std::make_tuple(parameters...);
    GError*      raw_error {nullptr};
    GUnixFDList* out_fd_list {nullptr};

    g_unix_fd_list_ptr fd_list {extract_g_unix_fd_list(fn_args)};

    g_variant_ptr result {g_dbus_proxy_call_with_unix_fd_list_sync(proxy_, method_name.c_str(), to_gvariant(fn_args),
                                                                   G_DBUS_CALL_FLAGS_NONE, -1, fd_list.get(),
                                                                   &out_fd_list, nullptr, &raw_error),
                          g_variant_unref};

    g_error_ptr        error {raw_error, g_error_free};
    g_unix_fd_list_ptr out_fd_list_raii_holder {out_fd_list, g_object_unref};

    if (!result)
        throw std::runtime_error("Proxy method call error: " + std::string {error ? error->message : "unknown error"});

    if constexpr (!std::is_void_v<R>) {
        if constexpr (is_tuple_like_v<R>) {
            auto ret = checked_from_gvariant<R>(result.get());
            set_up_from_g_unix_fd_list(out_fd_list, ret);
            return ret;
        } else {
            auto ret = checked_from_gvariant<std::tuple<R>>(result.get());
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

    return checked_from_gvariant<T>(result.get());
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

#endif // EASYDBUSPP_PROXY_INL_INCLUDED
