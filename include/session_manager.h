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

#ifndef __SESSION_MANAGER_H_INCLUDED__
#define __SESSION_MANAGER_H_INCLUDED__

#include "types.h"
#include <functional>
#include <gio/gio.h>
#include <memory>
#include <string>
#include <unordered_map>
#include <unordered_set>

namespace easydbuspp {

class object;
class proxy;

class session_manager {

    using signal_handler_t = std::function<void(GVariant*)>;

public:
    session_manager(GBusType bus_type);
    session_manager(GBusType bus_type, const std::string& bus_name);
    ~session_manager();

    session_manager(const session_manager&)            = delete;
    session_manager& operator=(const session_manager&) = delete;

    template <typename C>
    void signal_subscribe(const std::string& signal_name, C&& callable, const std::string& sender = {},
                          const std::string& interface_name = {}, const object_path_t& object_path = {});

    void run();
    void stop();

private:
    void attach(object* object_ptr);
    void detach(object* object_ptr);

    template <typename C, typename... A>
    signal_handler_t generate_signal_handler(C&& callable, const std::function<void(A...)>&);

    static void on_bus_acquired(GDBusConnection* connection, const gchar* name, gpointer user_data);
    static void on_name_acquired(GDBusConnection* connection, const gchar* name, gpointer user_data);
    static void on_name_lost(GDBusConnection* connection, const gchar* name, gpointer user_data);

    static void on_signal(GDBusConnection* connection, const gchar* sender_name, const gchar* object_path,
                          const gchar* interface_name, const gchar* signal_name, GVariant* parameters,
                          gpointer user_data);

    static int stop_sighandler(void* loop);

private:
    guint                                             owner_id_ {0};
    static inline GMainLoop*                          loop_ {nullptr};
    std::string                                       bus_name_;
    std::unordered_set<object*>                       objects_;
    std::unordered_map<std::string, signal_handler_t> signal_handlers_;
    GDBusConnection*                                  connection_ {nullptr};

    friend class object;
    friend class proxy;
};

} // end of namespace easydbuspp

#include "session_manager.inl"

#endif // __SESSION_MANAGER_H_INCLUDED__
