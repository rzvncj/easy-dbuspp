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

#include "org_freedesktop_dbus_proxy.h"
#include <stdexcept>

namespace easydbuspp {

org_freedesktop_dbus_proxy::org_freedesktop_dbus_proxy(session_manager& session_mgr)
    : proxy(session_mgr, "org.freedesktop.DBus", "org.freedesktop.DBus", "/net/freedesktop/DBus")
{
}

std::string org_freedesktop_dbus_proxy::unique_bus_name(const std::string& well_known_bus_name) const
{
    return call<std::string>("GetNameOwner", well_known_bus_name);
}

uid_t org_freedesktop_dbus_proxy::uid(const std::string& well_known_bus_name) const
{
    return call<uint32_t>("GetConnectionUnixUser", well_known_bus_name);
}

uid_t org_freedesktop_dbus_proxy::pid(const std::string& well_known_bus_name) const
{
    return call<uint32_t>("GetConnectionUnixProcessID", well_known_bus_name);
}

} // end of namespace easydbuspp
