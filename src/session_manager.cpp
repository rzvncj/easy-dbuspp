// SPDX-FileCopyrightText: © 2024 Răzvan Cojocaru <razvanc@mailbox.org>
//
// SPDX-License-Identifier: AGPL-3.0-only

#include <main_loop.h>
#include <object.h>
#include <session_manager.h>

namespace easydbuspp {

session_manager::session_manager(bus_type_t bus_type)
{
    GError* raw_error {nullptr};

    connection_ = g_bus_get_sync(to_g_bus_type(bus_type), nullptr, &raw_error);
    g_error_ptr error {raw_error, g_error_free};

    if (!connection_)
        throw std::runtime_error("Can't connect to the bus: " + std::string {error->message});
}

session_manager::session_manager(bus_type_t bus_type, const std::string& bus_name)
    : owner_id_ {g_bus_own_name(
          to_g_bus_type(bus_type), bus_name.c_str(),
          static_cast<GBusNameOwnerFlags>(G_BUS_NAME_OWNER_FLAGS_ALLOW_REPLACEMENT | G_BUS_NAME_OWNER_FLAGS_REPLACE),
          on_bus_acquired, on_name_acquired, on_name_lost, this, nullptr)},
      bus_name_ {bus_name}
{
}

session_manager::~session_manager()
{
    if (connection_) {
        for (auto&& object_ptr : objects_)
            object_ptr->disconnect();
    }

    if (owner_id_ != 0)
        g_bus_unown_name(owner_id_);

    if (connection_)
        g_object_unref(connection_);
}

void session_manager::attach(object* object_ptr)
{
    if (!object_ptr)
        throw std::runtime_error("Won't register a nullptr object!");

    objects_.insert(object_ptr);
}

void session_manager::detach(object* object_ptr)
{
    if (object_ptr) {
        object_ptr->disconnect();
        objects_.erase(object_ptr);
    }
}

void session_manager::on_bus_acquired(GDBusConnection* connection, const gchar* /* name */, gpointer user_data)
{
    auto* manager        = static_cast<session_manager*>(user_data);
    manager->connection_ = connection;

    for (auto&& obj_ptr : manager->objects_)
        obj_ptr->connect();
}

void session_manager::on_name_acquired(GDBusConnection* connection, const gchar* /* name */, gpointer user_data)
{
    auto* manager        = static_cast<session_manager*>(user_data);
    manager->connection_ = connection;
}

void session_manager::on_name_lost(GDBusConnection* connection, const gchar* /* name */, gpointer user_data)
{
    auto* manager        = static_cast<session_manager*>(user_data);
    manager->connection_ = connection;

    const std::lock_guard lock {manager->name_lost_handler_mutex_};

    if (manager->name_lost_handler_)
        manager->name_lost_handler_(manager->bus_name_);
    else
        g_critical("Lost D-Bus name '%s' (is another application that owns it already running?)",
                   manager->bus_name_.c_str());

    main_loop::instance().stop();
}

void session_manager::name_lost_handler(const name_lost_handler_t& handler)
{
    const std::lock_guard lock {name_lost_handler_mutex_};
    name_lost_handler_ = handler;
}

void session_manager::on_signal(GDBusConnection* /* connection */, const gchar* /* sender_name */,
                                const gchar* /* object_path */, const gchar* /* interface_name */,
                                const gchar* signal_name, GVariant* parameters, gpointer user_data)
{
    auto* manager = static_cast<session_manager*>(user_data);

    signal_handler_t handler;

    {
        const std::lock_guard lock {manager->signal_handlers_mutex_};
        auto                  it = manager->signal_handlers_.find(signal_name);

        if (it == manager->signal_handlers_.end()) {
            g_warning("No signal handler registered for '%s'", signal_name);
            return;
        }

        handler = it->second;
    }

    try {
        handler(parameters);
    } catch (const std::exception& e) {
        g_warning("Exception in signal handler for '%s': %s", signal_name, e.what());
    }
}

} // end of namespace easydbuspp
