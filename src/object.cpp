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

#include <object.h>
#include <stdexcept>

namespace easydbuspp {

g_thread_pool object::thread_pool_ {g_thread_pool_function};

object::object(session_manager& session_mgr, const std::string& interface_name, const object_path_t& object_path)
    : session_manager_ {session_mgr}, interface_name_ {interface_name}, object_path_ {object_path},
      introspection_data_ {nullptr, g_dbus_node_info_unref}
{
    session_manager_.attach(this);
}

object::~object()
{
    session_manager_.detach(this);
}

std::string object::introspection_xml() const
{
    return "<node>\n <interface name='" + interface_name_ + "'>\n" + methods_xml_ + properties_xml_ + signals_xml_
        + " </interface>\n</node>";
}

std::string object::interface_name() const
{
    return interface_name_;
}

object_path_t object::object_path() const
{
    return object_path_;
}

void object::pre_request_handler(const pre_request_handler_t& handler)
{
    pre_request_handler_ = handler;
}

void object::connect()
{
    if (!session_manager_.connection_)
        throw std::runtime_error("Invalid input when attempting object connect");

    GError* error {nullptr};

    introspection_data_ = g_dbus_node_info_ptr {g_dbus_node_info_new_for_xml(introspection_xml().c_str(), &error),
                                                g_dbus_node_info_unref};

    if (!introspection_data_) {
        std::string error_message = error->message;
        g_error_free(error);

        throw std::runtime_error("Could not initialize introspection XML: " + error_message);
    }

    registration_id_ = g_dbus_connection_register_object(
        session_manager_.connection_, object_path_.generic_string().c_str(), introspection_data_->interfaces[0],
        &interface_vtable_, this, nullptr, nullptr);
}

void object::disconnect()
{
    if (!session_manager_.connection_)
        return;

    g_dbus_connection_unregister_object(session_manager_.connection_, registration_id_);
    registration_id_             = 0;
    session_manager_.connection_ = nullptr;
}

void object::handle_method_call(GDBusConnection* /* connection */, const gchar* sender, const gchar* object_path,
                                const gchar* interface_name, const gchar* method_name, GVariant* parameters,
                                GDBusMethodInvocation* invocation, gpointer user_data)
{
    using namespace std::string_literals;

    dbus_context context {sender, interface_name, object_path, method_name};

    thread_pool_.push(new std::function<void()> {[=] {
        try {
            object* obj_ptr = static_cast<object*>(user_data);

            auto it = obj_ptr->methods_.find(method_name);

            if (it == obj_ptr->methods_.end())
                throw std::runtime_error("No method '"s + method_name + "' registered by object '"
                                         + obj_ptr->object_path_.generic_string() + "'!");

            if (obj_ptr->pre_request_handler_)
                obj_ptr->pre_request_handler_(request_type::METHOD, context);

            GVariant* ret = it->second(parameters, context);
            g_dbus_method_invocation_return_value(invocation, ret);

        } catch (const std::exception& e) {
            const std::string error_name {std::string {interface_name} + ".MethodError"};
            g_dbus_method_invocation_return_dbus_error(invocation, error_name.c_str(), e.what());
        }
    }});
}

GVariant* object::handle_get_property(GDBusConnection* /* connection */, const gchar* sender, const gchar* object_path,
                                      const gchar* interface_name, const gchar* property_name, GError** error,
                                      gpointer user_data)
{
    using namespace std::string_literals;

    try {
        object* obj_ptr = static_cast<object*>(user_data);

        auto it = obj_ptr->properties_.find(property_name);

        if (it == obj_ptr->properties_.end())
            throw std::runtime_error("No property '"s + property_name + "' registered by object '"
                                     + obj_ptr->object_path_.generic_string() + "'!");

        auto&& [getter, setter] = it->second;

        if (!getter)
            throw std::runtime_error("Property '"s + property_name + "' for object '"
                                     + obj_ptr->object_path_.generic_string() + "' cannot be read!");

        dbus_context context {sender, interface_name, object_path, property_name};

        if (obj_ptr->pre_request_handler_)
            obj_ptr->pre_request_handler_(request_type::GET_PROPERTY, context);

        return getter();

    } catch (const std::exception& e) {
        g_set_error(error, G_DBUS_ERROR, G_DBUS_ERROR_FAILED, "%s", e.what());
        return nullptr;
    }
}

gboolean object::handle_set_property(GDBusConnection* /* connection */, const gchar* sender, const gchar* object_path,
                                     const gchar* interface_name, const gchar* property_name, GVariant* value,
                                     GError** error, gpointer user_data)
{
    using namespace std::string_literals;

    try {
        object* obj_ptr = static_cast<object*>(user_data);

        auto it = obj_ptr->properties_.find(property_name);

        if (it == obj_ptr->properties_.end())
            throw std::runtime_error("No property '"s + property_name + "' registered by object '"
                                     + obj_ptr->object_path_.generic_string() + "'!");

        auto&& [getter, setter] = it->second;

        if (!setter)
            throw std::runtime_error("Property '"s + property_name + "' for object '"
                                     + obj_ptr->object_path_.generic_string() + "' is read only!");

        dbus_context context {sender, interface_name, object_path, property_name};

        if (obj_ptr->pre_request_handler_)
            obj_ptr->pre_request_handler_(request_type::SET_PROPERTY, context);

        return setter(value);

    } catch (const std::exception& e) {
        g_set_error(error, G_DBUS_ERROR, G_DBUS_ERROR_FAILED, "%s", e.what());
        return FALSE;
    }
}

void object::g_thread_pool_function(gpointer data, gpointer /* user_data */)
{
    using threadpool_fn_t = std::function<void()>;

    std::unique_ptr<threadpool_fn_t> method_ptr {static_cast<threadpool_fn_t*>(data)};
    (*method_ptr)();
}

void object::emit_properties_update_signal(const std::string& property_name, GVariant* value) const
{
    g_variant_builder_ptr builder {g_variant_builder_new(G_VARIANT_TYPE_ARRAY), g_variant_builder_unref};
    g_variant_builder_add(builder.get(), "{sv}", property_name.c_str(), value);
    GVariant* property_update = g_variant_new("(sa{sv}as)", interface_name_.c_str(), builder.get(), nullptr);

    g_dbus_connection_emit_signal(session_manager_.connection_, nullptr, object_path_.generic_string().c_str(),
                                  "org.freedesktop.DBus.Properties", "PropertiesChanged", property_update, nullptr);
}

} // end of namespace easydbuspp
