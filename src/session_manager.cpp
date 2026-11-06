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

session_manager::~session_manager() noexcept
{
    std::vector<guint> subscription_ids;

    {
        const std::lock_guard lock {signal_handlers_mutex_};
        subscription_ids.swap(signal_subscription_ids_);
    }

    if (connection_) {
        for (const guint id : subscription_ids)
            g_dbus_connection_signal_unsubscribe(connection_, id);
    }

    // The connection is shared and outlives us, so signals may still be in flight. Wait for them.
    lifetime_->close();

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
    auto* manager = static_cast<session_manager*>(user_data);

    // The connection passed to g_bus_own_name callbacks is not owned by us.
    // Take our own reference so we can safely g_object_unref() in the destructor.
    if (connection && !manager->connection_) {
        manager->connection_ = static_cast<GDBusConnection*>(g_object_ref(connection));
    }

    for (auto&& obj_ptr : manager->objects_) {
        try {
            obj_ptr->connect();
        } catch (const std::exception& e) {
            g_critical("Could not connect object '%s': %s", obj_ptr->object_path().c_str(), e.what());
        } catch (...) {
            g_critical("Could not connect object '%s': unknown exception", obj_ptr->object_path().c_str());
        }
    }
}

void session_manager::on_name_acquired(GDBusConnection* connection, const gchar* /* name */, gpointer user_data)
{
    auto* manager = static_cast<session_manager*>(user_data);

    // Guard: only take a ref if on_bus_acquired hasn't already set it.
    if (connection && !manager->connection_)
        manager->connection_ = static_cast<GDBusConnection*>(g_object_ref(connection));
}

void session_manager::on_name_lost(GDBusConnection* connection, const gchar* /* name */, gpointer user_data)
{
    auto* manager = static_cast<session_manager*>(user_data);

    // Guard: only take a ref if we don't already have one.
    if (connection && !manager->connection_)
        manager->connection_ = static_cast<GDBusConnection*>(g_object_ref(connection));

    const std::lock_guard lock {manager->name_lost_handler_mutex_};

    if (manager->name_lost_handler_) {
        try {
            manager->name_lost_handler_(manager->bus_name_);
        } catch (const std::exception& e) {
            g_warning("Exception in name_lost_handler: %s", e.what());
        } catch (...) {
            g_warning("Unknown exception in name_lost_handler");
        }
    } else {
        g_critical("Lost D-Bus name '%s' (is another application that owns it already running?)",
                   manager->bus_name_.c_str());
        main_loop::instance().stop();
    }
}

void session_manager::name_lost_handler(const name_lost_handler_t& handler)
{
    const std::lock_guard lock {name_lost_handler_mutex_};
    name_lost_handler_ = handler;
}

void session_manager::subscribe_to_signal(const std::string& signal_name, const std::string& sender,
                                          const std::string& interface_name, const object_path_t& object_path)
{
    if (!connection_) {
        g_critical("Can't subscribe to signal '%s': there is no D-Bus connection yet", signal_name.c_str());
        return;
    }

    // GDBus keeps this (and calls free_lifetime_guard_ptr()) until the subscription is gone and no
    // delivery is pending anymore.
    auto* subscription_data = new lifetime_guard_ptr {lifetime_};

    const guint id = g_dbus_connection_signal_subscribe(
        connection_, sender.empty() ? nullptr : sender.c_str(),
        interface_name.empty() ? nullptr : interface_name.c_str(), signal_name.c_str(),
        object_path.empty() ? nullptr : object_path.generic_string().c_str(), nullptr, G_DBUS_SIGNAL_FLAGS_NONE,
        on_signal, subscription_data, free_lifetime_guard_ptr);

    const std::lock_guard lock {signal_handlers_mutex_};
    signal_subscription_ids_.push_back(id);
}

void session_manager::on_signal(GDBusConnection* /* connection */, const gchar* /* sender_name */,
                                const gchar* /* object_path */, const gchar* /* interface_name */,
                                const gchar* signal_name, GVariant* parameters, gpointer user_data)
{
    const auto guard   = *static_cast<lifetime_guard_ptr*>(user_data);
    auto*      manager = guard->enter<session_manager>();

    if (!manager)
        return;

    const request_scope scope {*guard};

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
    } catch (...) {
        g_warning("Unknown exception in signal handler for '%s'", signal_name);
    }
}

} // end of namespace easydbuspp
