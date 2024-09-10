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
#include <session_manager.h>

namespace easydbuspp {

session_manager::session_manager(bus_type_t bus_type)
{
    GError* error {nullptr};

    connection_ = g_bus_get_sync(to_g_bus_type(bus_type), nullptr, &error);

    if (!connection_) {
        std::string error_message = error->message;
        g_error_free(error);

        throw std::runtime_error("Can't connect to the bus: " + error_message);
    }
}

session_manager::session_manager(bus_type_t bus_type, const std::string& bus_name) : bus_name_ {bus_name}
{
    owner_id_ = g_bus_own_name(
        to_g_bus_type(bus_type), bus_name.c_str(),
        static_cast<GBusNameOwnerFlags>(G_BUS_NAME_OWNER_FLAGS_ALLOW_REPLACEMENT | G_BUS_NAME_OWNER_FLAGS_REPLACE),
        on_bus_acquired, on_name_acquired, on_name_lost, this, nullptr);
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
    session_manager* manager = static_cast<session_manager*>(user_data);
    manager->connection_     = connection;

    for (auto&& obj_ptr : manager->objects_)
        obj_ptr->connect();
}

void session_manager::on_name_acquired(GDBusConnection* connection, const gchar* /* name */, gpointer user_data)
{
    session_manager* manager = static_cast<session_manager*>(user_data);
    manager->connection_     = connection;
}

void session_manager::on_name_lost(GDBusConnection* connection, const gchar* /* name */, gpointer user_data)
{
    session_manager* manager = static_cast<session_manager*>(user_data);
    manager->connection_     = connection;
    throw std::runtime_error("Lost D-Bus name (is another application that owns it already running?)");
}

void session_manager::on_signal(GDBusConnection* /* connection */, const gchar* /* sender_name */,
                                const gchar* /* object_path */, const gchar* /* interface_name */,
                                const gchar* signal_name, GVariant* parameters, gpointer user_data)
{
    using namespace std::string_literals;

    session_manager* manager = static_cast<session_manager*>(user_data);
    auto             it      = manager->signal_handlers_.find(signal_name);

    if (it == manager->signal_handlers_.end())
        throw std::runtime_error("No signal handler registered for ' + signal_name + ''!");

    it->second(parameters);
}

} // end of namespace easydbuspp
