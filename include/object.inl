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

#ifndef __OBJECT_INL_INCLUDED__
#define __OBJECT_INL_INCLUDED__

#include "session_manager.h"

namespace easydbuspp {

template <typename C, typename R, typename... A>
object::method_handler_t object::add_method_helper(const std::string& name, C&& callable, const std::function<R(A...)>&,
                                                   const std::vector<std::string>& in_argument_names,
                                                   const std::vector<std::string>& out_argument_names)
{
    if constexpr (!std::is_void_v<R>) {
        if constexpr (is_tuple_like_v<R>) {
            if (!out_argument_names.empty() && out_argument_names.size() != std::tuple_size_v<R>)
                throw std::runtime_error("Method '" + name
                                         + "': number of out argument names does not match output tuple size!");
        } else if (!out_argument_names.empty() && out_argument_names.size() != 1)
            throw std::runtime_error("Method '" + name + "': too many out argument names!");
    }

    size_t      arg_index {0};
    std::string method_xml {"  <method name='" + name + "'>\n"};

    (
        [&]() {
            if constexpr (!std::is_same_v<std::decay_t<A>, dbus_context>) {
                std::string arg_name = "in_arg" + std::to_string(arg_index);

                if (!in_argument_names.empty()) {
                    if (arg_index == in_argument_names.size())
                        throw std::runtime_error("Method '" + name + "': too few input argument names provided!");
                    arg_name = in_argument_names[arg_index];
                }

                method_xml
                    += "   <arg name='" + arg_name + "' type='" + to_dbus_type_string<A>() + "' direction='in'/>\n";

                ++arg_index;
            }
        }(),
        ...);

    if (!in_argument_names.empty() && in_argument_names.size() != arg_index)
        throw std::runtime_error("Method '" + name
                                 + "': number of input argument names does not match number of arguments!");

    if constexpr (!std::is_void_v<R>) {
        if constexpr (is_tuple_like_v<R>) {
            R output;
            arg_index = 0;

            std::apply(
                [&method_xml, &out_argument_names, &arg_index](auto&&... args) {
                    ((method_xml += "   <arg name='"
                          + (out_argument_names.empty() ? "out_arg" + std::to_string(arg_index++)
                                                        : out_argument_names[arg_index++])
                          + "' type='" + to_dbus_type_string(args) + "' direction='out'/>\n"),
                     ...);
                },
                output);
        } else
            method_xml += "   <arg name='" + (out_argument_names.empty() ? "out_arg0" : out_argument_names[0])
                + "' type='" + to_dbus_type_string<R>() + "' direction='out'/>\n";
    }

    method_xml += "  </method>\n";
    methods_xml_ += method_xml;

    return [callable](GVariant* parameters, const dbus_context& context) {
        std::tuple<std::decay_t<A>...> fn_args;
        gsize                          arg_index {0};

        // Initialize the tuple
        std::apply(
            [parameters, &arg_index, &context](auto&&... args) {
                (
                    [&]() {
                        if constexpr (std::is_same_v<std::decay_t<decltype(args)>, dbus_context>)
                            args = context;
                        else
                            args = extract<decltype(args)>(parameters, arg_index++);
                    }(),
                    ...);
            },
            fn_args);

        if constexpr (!std::is_void_v<R>) {
            if constexpr (is_tuple_like_v<R>)
                return to_gvariant(std::apply(callable, fn_args));
            else {
                std::tuple<R> wrapper {std::apply(callable, fn_args)};
                return to_gvariant(wrapper);
            }
        } else {
            std::apply(callable, fn_args);
            return nullptr;
        }
    };
}

template <typename C>
void object::add_method(const std::string& name, C&& callable, const std::vector<std::string>& in_argument_names,
                        const std::vector<std::string>& out_argument_names)
{
    using std_function_type = decltype(std::function {std::forward<C>(callable)});

    auto handler   = add_method_helper(name, std::forward<C>(callable), std_function_type {}, in_argument_names,
                                       out_argument_names);
    methods_[name] = handler;
}

template <typename... A>
std::function<void(A...)> object::add_broadcast_signal(const std::string&              name,
                                                       const std::vector<std::string>& argument_names)
{
    auto ret = add_signal<A...>(name, false, argument_names);

    return [ret](A... args) {
        ret("", args...);
    };
}

template <typename... A>
std::function<void(const std::string&, A...)> object::add_unicast_signal(const std::string&              name,
                                                                         const std::vector<std::string>& argument_names)
{
    return add_signal<A...>(name, true, argument_names);
}

template <typename... A>
std::function<void(const std::string&, A...)> object::add_signal(const std::string& name, bool unicast,
                                                                 const std::vector<std::string>& argument_names)
{
    if (!argument_names.empty() && argument_names.size() != sizeof...(A))
        throw std::runtime_error("Signal '" + name
                                 + "': number of in argument names does not match number of arguments!");

    std::string signal_xml {"  <signal name='" + name + "'>\n"};

    int arg_index = 0;

    ((signal_xml += "   <arg type='" + to_dbus_type_string<A>() + "' name='"
          + (argument_names.empty() ? "arg" + std::to_string(arg_index++) : argument_names[arg_index++]) + "'/>\n"),
     ...);

    signal_xml += "  </signal>\n";
    signals_xml_ += signal_xml;

    return [this, name, unicast](const std::string& bus_name, A... args) {
        if (!session_manager_.connection_)
            throw std::runtime_error("Unable to send signal '" + name
                                     + "': the D-Bus connection needs to be established first!");

        std::tuple<A...> fn_args(args...);

        g_dbus_connection_emit_signal(session_manager_.connection_, unicast ? bus_name.c_str() : nullptr,
                                      object_path_.c_str(), interface_name_.c_str(), name.c_str(), to_gvariant(fn_args),
                                      nullptr);
    };
}

template <typename T>
void object::add_property(const std::string& name, T&& value)
{
    std::string property_xml {"  <property name='" + name + "' type='" + to_dbus_type_string<T>() + "' access='"};

    if constexpr (!is_output_type_v<T>) {
        property_xml += "read";
        properties_[name] = std::pair<property_read_handler_t, property_write_handler_t>(
            [value] {
                return to_gvariant(value);
            },
            {});
    } else {
        property_xml += "readwrite";
        properties_[name] = std::pair<property_read_handler_t, property_write_handler_t>(
            [&value] {
                return to_gvariant(value);
            },
            [this, &value, name](GVariant* new_value) {
                value = from_gvariant<T>(new_value);
                emit_properties_update_signal(name, new_value);
                return TRUE;
            });
    }

    property_xml += "'/>\n";
    properties_xml_ += property_xml;
}

template <typename T>
void object::add_property(const std::string& name, const std::function<T()>& getter,
                          const std::function<bool(const T&)>& setter)
{
    if (!setter && !getter)
        throw std::runtime_error("Property '" + name + "': needs to provide at least a setter or a getter!");

    std::string property_xml {"  <property name='" + name + "' type='" + to_dbus_type_string<T>() + "' access='"};

    if (!setter)
        property_xml += "read";
    else if (!getter)
        property_xml += "write";
    else
        property_xml += "readwrite";

    property_xml += "'/>\n";
    properties_xml_ += property_xml;

    auto read_lambda = [getter] {
        return to_gvariant(getter());
    };

    auto write_lambda = [this, setter, name](GVariant* new_value) {
        gboolean ret = setter(from_gvariant<T>(new_value));
        emit_properties_update_signal(name, new_value);
        return ret;
    };

    properties_[name] = std::make_pair(getter ? read_lambda : property_read_handler_t {},
                                       setter ? write_lambda : property_write_handler_t {});
}

} // end of namespace easydbuspp

#endif // __OBJECT_INL_INCLUDED__
