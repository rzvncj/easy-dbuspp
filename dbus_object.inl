#ifndef __DBUS_OBJECT_INL_INCLUDED__
#define __DBUS_OBJECT_INL_INCLUDED__

template <typename C, typename R, typename... A>
dbus_object::dbus_handler dbus_object::add_method_helper(const std::string& name, C&& callable,
                                                         const std::function<R(A...)>&,
                                                         const std::vector<std::string>& in_argument_names,
                                                         const std::vector<std::string>& out_argument_names)
{
    static_assert((... && !is_output_type_v<A>), "Only input parameters allowed (no non-const references)!");

    if (!in_argument_names.empty() && in_argument_names.size() != sizeof...(A))
        throw std::runtime_error("Method '" + name
                                 + "': number of in argument names does not match number of arguments!");

    if constexpr (!std::is_void_v<R>) {
        if constexpr (is_tuple_v<R>) {
            if (!out_argument_names.empty() && out_argument_names.size() != std::tuple_size_v<R>)
                throw std::runtime_error("Method '" + name
                                         + "': number of out argument names does not match output tuple size!");
        } else if (!out_argument_names.empty() && out_argument_names.size() != 1)
            throw std::runtime_error("Method '" + name + "': too many out argument names!");
    }

    int         arg_index {0};
    std::string method_xml {"  <method name='" + name + "'>\n"};

    ((method_xml += "   <arg name='"
          + (in_argument_names.empty() ? "in_arg" + std::to_string(arg_index++) : in_argument_names[arg_index++])
          + "' type='" + to_dbus_type<A>() + "' direction='in'/>\n"),
     ...);

    if constexpr (!std::is_void_v<R>) {
        if constexpr (is_tuple_v<R>) {
            R output;
            arg_index = 0;

            std::apply(
                [&method_xml, &out_argument_names, &arg_index](auto&&... args) {
                    ((method_xml += "   <arg name='"
                          + (out_argument_names.empty() ? "out_arg" + std::to_string(arg_index++)
                                                        : out_argument_names[arg_index++])
                          + "' type='" + to_dbus_type(args) + "' direction='out'/>\n"),
                     ...);
                },
                output);
        } else
            method_xml += "   <arg name='" + (out_argument_names.empty() ? "out_arg0" : out_argument_names[0])
                + "' type='" + to_dbus_type<R>() + "' direction='out'/>\n";
    }

    method_xml += "  </method>\n";
    methods_xml_ += method_xml;

    return [callable](GVariant* parameters) {
        std::tuple<std::decay_t<A>...> fn_args;
        gsize                          arg_index {0};
        GVariant*                      ret {nullptr};

        // Initialize the tuple
        std::apply(
            [parameters, &arg_index](auto&&... args) {
                ((args = extract<decltype(args)>(parameters, arg_index++)), ...);
            },
            fn_args);

        if constexpr (!std::is_void_v<R>) {
            if constexpr (is_tuple_v<R>)
                ret = to_gvariant(std::apply(callable, fn_args));
            else {
                std::tuple<R> wrapper {std::apply(callable, fn_args)};
                ret = to_gvariant(wrapper);
            }
        } else
            std::apply(callable, fn_args);

        return ret;
    };
}

template <typename C>
void dbus_object::add_method(const std::string& name, C&& callable, const std::vector<std::string>& in_argument_names,
                             const std::vector<std::string>& out_argument_names)
{
    using std_function_type = decltype(std::function {std::forward<C>(callable)});

    auto handler   = add_method_helper(name, std::forward<C>(callable), std_function_type {}, in_argument_names,
                                       out_argument_names);
    methods_[name] = handler;
}

#endif // __DBUS_OBJECT_INL_INCLUDED__
