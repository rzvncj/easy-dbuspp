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

#ifndef __PROXY_INL_INCLUDED__
#define __PROXY_INL_INCLUDED__

namespace easydbuspp {

template <typename R, typename... A>
R proxy::call(const std::string& method_name, A... parameters) const
{
    std::tuple<std::decay_t<A>...> fn_args {parameters...};
    GError*                        error {nullptr};

    g_variant_ptr result {g_dbus_proxy_call_sync(proxy_, method_name.c_str(), to_gvariant(fn_args),
                                                 G_DBUS_CALL_FLAGS_NONE, -1, nullptr, &error),
                          g_variant_unref};

    if (!result) {
        std::string error_message = error->message;
        g_error_free(error);

        throw std::runtime_error("Proxy method call error: " + error_message);
    }

    if constexpr (!std::is_void_v<R>) {
        if constexpr (is_tuple_like_v<R>)
            return from_gvariant<R>(result.get());
        else
            return std::get<0>(from_gvariant<std::tuple<R>>(result.get()));
    }
}

template <typename T>
T proxy::cached_property(const std::string& property_name) const
{
    g_variant_ptr result {g_dbus_proxy_get_cached_property(proxy_, property_name.c_str()), g_variant_unref};

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
    props_proxy.call<void>("Set", interface_name_, property_name, std::variant<T> {new_value});
}

} // end of namespace easydbuspp

#endif // __PROXY_INL_INCLUDED__
