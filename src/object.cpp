// SPDX-FileCopyrightText: © 2024 Răzvan Cojocaru <razvanc@mailbox.org>
//
// SPDX-License-Identifier: AGPL-3.0-only

#include <idle_detector.h>
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

object::~object() noexcept
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
    const std::lock_guard lock {pre_request_handler_mutex_};
    pre_request_handler_ = handler;
}

void object::connect()
{
    if (!session_manager_.connection_)
        throw std::runtime_error("Invalid input when attempting object connect");

    GError* raw_error {nullptr};

    introspection_data_ = g_dbus_node_info_ptr {g_dbus_node_info_new_for_xml(introspection_xml().c_str(), &raw_error),
                                                g_dbus_node_info_unref};
    g_error_ptr error {raw_error, g_error_free};

    if (!introspection_data_)
        throw std::runtime_error("Could not initialize introspection XML: " + std::string {error->message});

    registration_id_ = g_dbus_connection_register_object(
        session_manager_.connection_, object_path_.generic_string().c_str(), introspection_data_->interfaces[0],
        &interface_vtable_, this, nullptr, nullptr);
}

void object::disconnect()
{
    if (!session_manager_.connection_ || registration_id_ == 0)
        return;

    g_dbus_connection_unregister_object(session_manager_.connection_, registration_id_);
    registration_id_ = 0;
}

void object::handle_method_call(GDBusConnection* /* connection */, const gchar* sender, const gchar* object_path,
                                const gchar* interface_name, const gchar* method_name, GVariant* parameters,
                                GDBusMethodInvocation* invocation, gpointer user_data)
{
    using namespace std::string_literals;

    const dbus_context context {sender, interface_name, object_path, method_name};

    auto task = std::make_unique<std::function<void()>>([=] {
        try {
            auto* obj_ptr = static_cast<object*>(user_data);

            idle_detector::instance().ping(obj_ptr->object_path());

            auto it = obj_ptr->methods_.find(method_name);

            if (it == obj_ptr->methods_.end())
                throw std::runtime_error("No method '"s + method_name + "' registered by object '"
                                         + obj_ptr->object_path_.generic_string() + "'!");

            {
                pre_request_handler_t pre_handler;
                {
                    const std::lock_guard lock {obj_ptr->pre_request_handler_mutex_};
                    pre_handler = obj_ptr->pre_request_handler_;
                }
                if (pre_handler)
                    pre_handler(request_type::METHOD, context);
            }

            GDBusMessage* message = g_dbus_method_invocation_get_message(invocation);
            GUnixFDList*  fd_list = g_dbus_message_get_unix_fd_list(message);

            auto [ret, out_fd_list] = it->second(parameters, fd_list, context);
            g_dbus_method_invocation_return_value_with_unix_fd_list(invocation, ret, out_fd_list.get());

        } catch (const std::exception& e) {
            const std::string error_name {std::string {interface_name} + ".MethodError"};
            g_dbus_method_invocation_return_dbus_error(invocation, error_name.c_str(), e.what());
        }
    });

    thread_pool_.push(task.get());
    task.release();
}

GVariant* object::handle_get_property(GDBusConnection* /* connection */, const gchar* sender, const gchar* object_path,
                                      const gchar* interface_name, const gchar* property_name, GError** error,
                                      gpointer user_data)
{
    using namespace std::string_literals;

    try {
        object* obj_ptr = static_cast<object*>(user_data);

        idle_detector::instance().ping(obj_ptr->object_path());

        auto it = obj_ptr->properties_.find(property_name);

        if (it == obj_ptr->properties_.end())
            throw std::runtime_error("No property '"s + property_name + "' registered by object '"
                                     + obj_ptr->object_path_.generic_string() + "'!");

        auto&& [getter, setter] = it->second;

        if (!getter)
            throw std::runtime_error("Property '"s + property_name + "' for object '"
                                     + obj_ptr->object_path_.generic_string() + "' cannot be read!");

        const dbus_context context {sender, interface_name, object_path, property_name};

        {
            pre_request_handler_t pre_handler;
            {
                const std::lock_guard lock {obj_ptr->pre_request_handler_mutex_};
                pre_handler = obj_ptr->pre_request_handler_;
            }
            if (pre_handler)
                pre_handler(request_type::GET_PROPERTY, context);
        }

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
        auto* obj_ptr = static_cast<object*>(user_data);

        idle_detector::instance().ping(obj_ptr->object_path());

        auto it = obj_ptr->properties_.find(property_name);

        if (it == obj_ptr->properties_.end())
            throw std::runtime_error("No property '"s + property_name + "' registered by object '"
                                     + obj_ptr->object_path_.generic_string() + "'!");

        auto&& [getter, setter] = it->second;

        if (!setter)
            throw std::runtime_error("Property '"s + property_name + "' for object '"
                                     + obj_ptr->object_path_.generic_string() + "' is read only!");

        const dbus_context context {sender, interface_name, object_path, property_name};

        {
            pre_request_handler_t pre_handler;
            {
                const std::lock_guard lock {obj_ptr->pre_request_handler_mutex_};
                pre_handler = obj_ptr->pre_request_handler_;
            }
            if (pre_handler)
                pre_handler(request_type::SET_PROPERTY, context);
        }

        return setter(value);

    } catch (const std::exception& e) {
        g_set_error(error, G_DBUS_ERROR, G_DBUS_ERROR_FAILED, "%s", e.what());
        return FALSE;
    }
}

void object::g_thread_pool_function(gpointer data, gpointer /* user_data */)
{
    using threadpool_fn_t = std::function<void()>;

    const std::unique_ptr<threadpool_fn_t> method_ptr {static_cast<threadpool_fn_t*>(data)};
    (*method_ptr)();
}

void object::emit_properties_update_signal(const std::string& property_name, GVariant* value) const
{
    const g_variant_builder_ptr builder {g_variant_builder_new(G_VARIANT_TYPE_ARRAY), g_variant_builder_unref};
    g_variant_builder_add(builder.get(), "{sv}", property_name.c_str(), value);
    GVariant* property_update = g_variant_new("(sa{sv}as)", interface_name_.c_str(), builder.get(), nullptr);

    g_dbus_connection_emit_signal(session_manager_.connection_, nullptr, object_path_.generic_string().c_str(),
                                  "org.freedesktop.DBus.Properties", "PropertiesChanged", property_update, nullptr);
}

} // end of namespace easydbuspp
