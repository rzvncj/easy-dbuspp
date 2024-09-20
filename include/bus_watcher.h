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
#include <condition_variable>
#include <functional>
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
     * @param start    If this is true, try to automatically start the service that owns the
     *                 bus name, if activatable.
     */
    explicit bus_watcher(bus_type_t bus_type, const std::string& bus_name, bool start = false);

    //! Destructor.
    ~bus_watcher();

    bus_watcher(const bus_watcher&)            = delete;
    bus_watcher& operator=(const bus_watcher&) = delete;

public:
    //! Waits for a given period for the bus name to become available.
    template <typename Rep, typename Period>
    void wait_for(const std::chrono::duration<Rep, Period>& timeout);

    //! (Optionally) set a function to be called when the bus name disappears.
    void bus_name_disappeared_handler(const std::function<void()>& disappeared_handler);

private:
    static void on_name_appeared(GDBusConnection* connection, const gchar* name, const gchar* name_owner,
                                 gpointer user_data);

    static void on_name_disappeared(GDBusConnection* connection, const gchar* name, gpointer user_data);

private:
    std::condition_variable name_appeared_cv_;
    std::mutex              name_appeared_cv_mutex_;
    bool                    name_appeared_ {false};
    guint                   watcher_id_ {0};
    std::function<void()>   disappeared_handler_;
};

} // end of namespace easydbuspp

#include "bus_watcher.inl"

#endif // __BUS_WATCHER_H_INCLUDED__
