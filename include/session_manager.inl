// SPDX-FileCopyrightText: © 2024 Răzvan Cojocaru <rzvncj@gmail.com>
//
// SPDX-License-Identifier: AGPL-3.0-only

#ifndef __SESSION_MANAGER_INL_INCLUDED__
#define __SESSION_MANAGER_INL_INCLUDED__

#include "params.h"

namespace easydbuspp {

template <typename C, typename... A>
session_manager::signal_handler_t session_manager::generate_signal_handler(C&& callable,
                                                                           const std::function<void(A...)>&)
{
    return [callable](GVariant* parameters) {
        std::tuple<std::decay_t<A>...> fn_args;
        gsize                          arg_index {0};

        // Initialize the tuple
        std::apply(
            [parameters, &arg_index](auto&&... args) {
                ((args = extract<decltype(args)>(parameters, arg_index++)), ...);
            },
            fn_args);

        std::apply(callable, fn_args);
    };
}

template <typename C>
void session_manager::signal_subscribe(const std::string& signal_name, C&& callable, const std::string& sender,
                                       const std::string& interface_name, const object_path_t& object_path)
{
    using std_function_type = decltype(std::function {std::forward<C>(callable)});

    signal_handlers_[signal_name] = generate_signal_handler(std::forward<C>(callable), std_function_type {});

    g_dbus_connection_signal_subscribe(connection_, sender.empty() ? nullptr : sender.c_str(),
                                       interface_name.empty() ? nullptr : interface_name.c_str(), signal_name.c_str(),
                                       object_path.empty() ? nullptr : object_path.generic_string().c_str(), nullptr,
                                       G_DBUS_SIGNAL_FLAGS_NONE, on_signal, this, nullptr);
}

} // end of namespace easydbuspp

#endif // __SESSION_MANAGER_INL_INCLUDED__
