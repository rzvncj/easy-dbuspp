// SPDX-FileCopyrightText: © 2024 Răzvan Cojocaru <rzvncj@gmail.com>
//
// SPDX-License-Identifier: AGPL-3.0-only

#ifndef __ORG_FREEDESKTOP_DBUS_H_INCLUDED__
#define __ORG_FREEDESKTOP_DBUS_H_INCLUDED__

#include "proxy.h"
#include <cstdlib>
#include <functional>
#include <sys/types.h>
#include <utility>

namespace easydbuspp {

/*!
 * Utility class connected to `/net/freedesktop/DBus`.
 */
class org_freedesktop_dbus_proxy : public proxy {

public:
    //! Constructor. Connects to D-Bus via the session_manager's connection.
    explicit org_freedesktop_dbus_proxy(session_manager& session_mgr);

public:
    //! Get the unique bus name of a well-known bus name.
    std::string unique_bus_name(const std::string& well_known_bus_name) const;

    //! Get the UID of the process that owns `bus_name`.
    uid_t uid(const std::string& bus_name) const;

    //! Get the PID of the process that owns `bus_name`.
    pid_t pid(const std::string& bus_name) const;
};

} // end of namespace easydbuspp

#endif // __ORG_FREEDESKTOP_DBUS_H_INCLUDED__
