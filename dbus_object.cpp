#include "dbus_object.h"
#include <stdexcept>

dbus_object::dbus_object(const std::string& interface_name, const object_path_t& object_path)
    : interface_name_ {interface_name}, object_path_ {object_path}
{
}

std::string dbus_object::introspection_xml() const
{
    return "<node>\n <interface name='" + interface_name_ + "'>\n" + methods_xml_ + " </interface>\n</node>";
}

std::string dbus_object::interface_name() const
{
    return interface_name_;
}

object_path_t dbus_object::object_path() const
{
    return object_path_;
}

GVariant* dbus_object::call(const std::string& method_name, GVariant* parameters)
{
    auto it = methods_.find(method_name);

    if (it == methods_.end())
        throw std::runtime_error("No method '" + method_name + "' registered by object '"
                                 + object_path_.generic_string() + "'!");

    return it->second(parameters);
}
