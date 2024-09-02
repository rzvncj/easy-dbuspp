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

#ifndef __ORG_FREEDESKTOP_DBUS_H_INCLUDED__
#define __ORG_FREEDESKTOP_DBUS_H_INCLUDED__

#include "proxy.h"
#include <cstdlib>
#include <functional>
#include <sys/types.h>
#include <utility>

namespace easydbuspp {

class org_freedesktop_dbus_proxy : public proxy {

public:
    org_freedesktop_dbus_proxy(session_manager& session_manager);

public:
    std::string unique_bus_name(const std::string& well_known_bus_name) const;
    uid_t       uid(const std::string& well_known_bus_name) const;
    uid_t       pid(const std::string& well_known_bus_name) const;
};

} // end of namespace easydbuspp

#endif // __ORG_FREEDESKTOP_DBUS_H_INCLUDED__
