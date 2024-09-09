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

#ifndef __BUS_WATCHER_H_INCLUDED__
#define __BUS_WATCHER_H_INCLUDED__

#include "types.h"
#include <chrono>
#include <gio/gio.h>
#include <string>

namespace easydbuspp {

/*!
 * Utility class meant to wait until a bus name becomes available.
 */
class bus_watcher {

public:
    /*!
     * Constructor.
     *
     * @param bus_type The type of bus to listen on (session or system).
     * @param bus_name The name we want to make sure is available.
     */
    explicit bus_watcher(bus_type_t bus_type, const std::string& bus_name);

    //! Destructor.
    ~bus_watcher();

    bus_watcher(const bus_watcher&)            = delete;
    bus_watcher& operator=(const bus_watcher&) = delete;

public:
    //! Waits for a given period for the bus name to become available.
    template <typename Rep, typename Period>
    void wait_for(const std::chrono::duration<Rep, Period>& timeout);

private:
    static void on_name_appeared(GDBusConnection* connection, const gchar* name, const gchar* name_owner,
                                 gpointer user_data);

private:
    GMainLoop* loop_ {nullptr};
    guint      watcher_id_ {0};
};

} // end of namespace easydbuspp

#include "bus_watcher.inl"

#endif // __BUS_WATCHER_H_INCLUDED__
