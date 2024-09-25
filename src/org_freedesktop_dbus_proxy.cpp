// SPDX-FileCopyrightText: © 2024 Răzvan Cojocaru <rzvncj@gmail.com>
//
// SPDX-License-Identifier: AGPL-3.0-only

#include <org_freedesktop_dbus_proxy.h>
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

uid_t org_freedesktop_dbus_proxy::uid(const std::string& bus_name) const
{
    return call<uint32_t>("GetConnectionUnixUser", bus_name);
}

pid_t org_freedesktop_dbus_proxy::pid(const std::string& bus_name) const
{
    return call<uint32_t>("GetConnectionUnixProcessID", bus_name);
}

} // end of namespace easydbuspp
