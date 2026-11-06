// SPDX-FileCopyrightText: © 2024 Răzvan Cojocaru <razvanc@mailbox.org>
//
// SPDX-License-Identifier: AGPL-3.0-only

#ifndef EASYDBUSPP_SESSION_MANAGER_INL_INCLUDED
#define EASYDBUSPP_SESSION_MANAGER_INL_INCLUDED

#include "params.h"

namespace easydbuspp {

template <typename C, typename... A>
session_manager::signal_handler_t session_manager::generate_signal_handler(C&& callable,
                                                                           const std::function<void(A...)>&)
{
    // std::function needs copyable handlers, so a shared callable is what allows move-only and mutable ones.
    auto shared_callable = std::make_shared<std::decay_t<C>>(std::forward<C>(callable));

    return [shared_callable](GVariant* parameters) {
        // Anybody can emit a signal with any parameters, so don't trust them.
        if (!parameters || !g_variant_is_of_type(parameters, to_dbus_type<std::tuple<std::decay_t<A>...>>()))
            throw std::runtime_error {"Signal parameters are of type '"
                                      + std::string {parameters ? g_variant_get_type_string(parameters) : "(null)"}
                                      + "', expected '" + to_dbus_type_string<std::tuple<std::decay_t<A>...>>() + "'!"};

        std::tuple<std::decay_t<A>...> fn_args;
        gsize                          arg_index {0};

        // Initialize the tuple
        std::apply(
            [parameters, &arg_index](auto&&... args) {
                ((args = extract<decltype(args)>(parameters, arg_index++)), ...);
            },
            fn_args);

        std::apply(*shared_callable, fn_args);
    };
}

template <typename C>
void session_manager::signal_subscribe(const std::string& signal_name, C&& callable, const std::string& sender,
                                       const std::string& interface_name, const object_path_t& object_path)
{
    using std_function_type = decltype(std::function {std::forward<C>(callable)});

    subscribe_to_signal(signal_name, generate_signal_handler(std::forward<C>(callable), std_function_type {}), sender,
                        interface_name, object_path);
}

} // end of namespace easydbuspp

#endif // EASYDBUSPP_SESSION_MANAGER_INL_INCLUDED
