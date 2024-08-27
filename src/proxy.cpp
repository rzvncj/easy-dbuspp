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

#include "proxy.h"
#include "session_manager.h"
#include <stdexcept>

namespace easydbuspp {

proxy::proxy(session_manager& session_manager, const std::string& bus_name, const std::string& interface_name,
             const object_path_t& object_path)
    : session_manager_ {session_manager}, bus_name_ {bus_name}, interface_name_ {interface_name},
      object_path_ {object_path}
{
    GError* error {nullptr};

    if (!session_manager_.connection_)
        throw std::runtime_error("Could not create proxy: no live D-Bus connection!");

    proxy_ = g_dbus_proxy_new_sync(session_manager_.connection_, G_DBUS_PROXY_FLAGS_NONE,
                                   nullptr /* GDBusInterfaceInfo */, bus_name.c_str(),
                                   object_path.generic_string().c_str(), interface_name.c_str(), nullptr, &error);

    if (!proxy_) {
        std::string error_message = error->message;
        g_error_free(error);

        throw std::runtime_error("Could not create proxy: " + error_message);
    }
}

proxy::~proxy()
{
    g_object_unref(proxy_);
}

std::string proxy::unique_bus_name() const
{
    return g_dbus_connection_get_unique_name(g_dbus_proxy_get_connection(proxy_));
}

} // end of namespace easydbuspp
