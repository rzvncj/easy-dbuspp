#include "session_manager.h"

namespace {

} // end of anonymous namespace

dbus_session_manager::dbus_session_manager(GBusType bus_type, const std::string& bus_name)
{
    owner_id_ = g_bus_own_name(bus_type, bus_name.c_str(), G_BUS_NAME_OWNER_FLAGS_NONE, on_bus_acquired,
                               on_name_acquired, on_name_lost, this, nullptr);

    loop_ = g_main_loop_new(nullptr, FALSE);
}

dbus_session_manager::~dbus_session_manager()
{
    g_bus_unown_name(owner_id_);

    if (introspection_data_)
        g_dbus_node_info_unref(introspection_data_);
}

void dbus_session_manager::run()
{
    g_main_loop_run(loop_);
}

void dbus_session_manager::manage(std::shared_ptr<dbus_object> object_ptr)
{
    managed_objects_[object_ptr->object_path()] = object_ptr;
}

void dbus_session_manager::on_bus_acquired(GDBusConnection* connection, const gchar* name, gpointer user_data)
{
    dbus_session_manager* session_manager = static_cast<dbus_session_manager*>(user_data);

    std::string introspection_xml;

    for (auto&& [path, obj_ptr] : session_manager->managed_objects_)
        introspection_xml += obj_ptr->introspection_xml();

    introspection_data_ = g_dbus_node_info_new_for_xml(introspection_xml.c_str(), nullptr);

    int obj_index {0};

    for (auto&& [path, obj_ptr] : session_manager->managed_objects_)
        g_dbus_connection_register_object(connection, obj_ptr->object_path().generic_string().c_str(),
                                          introspection_data_->interfaces[obj_index++], &interface_vtable_, user_data,
                                          nullptr, nullptr);
}

void dbus_session_manager::on_name_acquired(GDBusConnection* connection, const gchar* name, gpointer user_data)
{
}

void dbus_session_manager::on_name_lost(GDBusConnection* connection, const gchar* name, gpointer user_data)
{
    exit(1);
}

void dbus_session_manager::handle_method_call(GDBusConnection* connection, const gchar* sender,
                                              const gchar* object_path, const gchar* interface_name,
                                              const gchar* method_name, GVariant* parameters,
                                              GDBusMethodInvocation* invocation, gpointer user_data)
{
    using namespace std::string_literals;

    dbus_session_manager* session_manager = static_cast<dbus_session_manager*>(user_data);

    auto it = session_manager->managed_objects_.find(object_path);

    if (it == session_manager->managed_objects_.end())
        throw std::runtime_error("No '"s + object_path + "' registered!");

    GVariant* ret = it->second->call(method_name, parameters);

    if (ret)
        g_dbus_method_invocation_return_value(invocation, ret);
}

GVariant* dbus_session_manager::handle_get_property(GDBusConnection* connection, const gchar* sender,
                                                    const gchar* object_path, const gchar* interface_name,
                                                    const gchar* property_name, GError** error, gpointer user_data)
{
    return nullptr;
}

gboolean dbus_session_manager::handle_set_property(GDBusConnection* connection, const gchar* sender,
                                                   const gchar* object_path, const gchar* interface_name,
                                                   const gchar* property_name, GVariant* value, GError** error,
                                                   gpointer user_data)
{
    return FALSE;
}
