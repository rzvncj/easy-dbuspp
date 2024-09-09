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

#include <glib-unix.h>
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

    setup_main_loop();
}

session_manager::session_manager(bus_type_t bus_type, const std::string& bus_name) : bus_name_ {bus_name}
{
    owner_id_ = g_bus_own_name(
        to_g_bus_type(bus_type), bus_name.c_str(),
        static_cast<GBusNameOwnerFlags>(G_BUS_NAME_OWNER_FLAGS_ALLOW_REPLACEMENT | G_BUS_NAME_OWNER_FLAGS_REPLACE),
        on_bus_acquired, on_name_acquired, on_name_lost, this, nullptr);

    setup_main_loop();
}

session_manager::~session_manager()
{
    destructor_called_ = true;

    if (connection_) {
        for (auto&& object_ptr : objects_)
            object_ptr->disconnect();
    }

    if (owner_id_ != 0)
        g_bus_unown_name(owner_id_);

    if (connection_)
        g_object_unref(connection_);

    stop();

    g_main_loop_unref(loop_);
}

void session_manager::run()
{
    g_main_loop_run(loop_);
}

void session_manager::run_async()
{
    if (run_future_.valid())
        return;

    run_future_ = std::async(std::launch::async, [this] {
        run();
    });
}

void session_manager::wait()
{
    if (run_future_.valid())
        run_future_.get();
}

void session_manager::stop()
{
    if (g_main_loop_is_running(loop_))
        g_main_loop_quit(loop_);

    if (!destructor_called_ && run_future_.valid())
        run_future_.get();
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

void session_manager::setup_main_loop()
{
    loop_ = g_main_loop_new(nullptr, FALSE);

    g_unix_signal_add(SIGINT, stop_sighandler, loop_);
    g_unix_signal_add(SIGTERM, stop_sighandler, loop_);
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

int session_manager::stop_sighandler(void* param)
{
    GMainLoop* loop = static_cast<GMainLoop*>(param);

    if (g_main_loop_is_running(loop))
        g_main_loop_quit(loop);

    return G_SOURCE_CONTINUE;
}

} // end of namespace easydbuspp
