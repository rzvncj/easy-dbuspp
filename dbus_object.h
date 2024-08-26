#ifndef __DBUS_OBJECT_H_INCLUDED__
#define __DBUS_OBJECT_H_INCLUDED__

#include "params.h"
#include "type_mapping.h"
#include "types.h"
#include <cstdlib>
#include <functional>
#include <gio/gio.h>

class dbus_object {

    using dbus_handler = std::function<GVariant*(GVariant*)>;

public:
    dbus_object(const std::string& interface_name, const object_path_t& object_path);

    template <typename C>
    void add_method(const std::string& name, C&& callable, const std::vector<std::string>& in_argument_names = {},
                    const std::vector<std::string>& out_argument_names = {});

    std::string   introspection_xml() const;
    std::string   interface_name() const;
    object_path_t object_path() const;

    GVariant* call(const std::string& method_name, GVariant* parameters);

private:
    template <typename C, typename R, typename... A>
    dbus_handler add_method_helper(const std::string& name, C&& callable, const std::function<R(A...)>&,
                                   const std::vector<std::string>& in_argument_names,
                                   const std::vector<std::string>& out_argument_names);

private:
    std::string                                   interface_name_;
    object_path_t                                 object_path_;
    std::string                                   methods_xml_;
    std::unordered_map<std::string, dbus_handler> methods_;
};

#include "dbus_object.inl"

#endif // __DBUS_OBJECT_H_INCLUDED__
