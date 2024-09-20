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

#include <bus_watcher.h>
#include <stdexcept>

namespace easydbuspp {

bus_watcher::bus_watcher(bus_type_t bus_type, const std::string& bus_name, bool start)
{
    watcher_id_ = g_bus_watch_name(to_g_bus_type(bus_type), bus_name.c_str(),
                                   start ? G_BUS_NAME_WATCHER_FLAGS_AUTO_START : G_BUS_NAME_WATCHER_FLAGS_NONE,
                                   on_name_appeared, on_name_disappeared, this, nullptr);
}

bus_watcher::~bus_watcher()
{
    g_bus_unwatch_name(watcher_id_);
}

void bus_watcher::bus_name_disappeared_handler(const std::function<void()>& disappeared_handler)
{
    std::lock_guard lock {name_appeared_cv_mutex_};
    disappeared_handler_ = disappeared_handler;
}

void bus_watcher::on_name_appeared(GDBusConnection* /* connection */, const gchar* /* name */,
                                   const gchar* /* name_owner */, gpointer user_data)
{
    bus_watcher* watcher = static_cast<bus_watcher*>(user_data);

    std::lock_guard lock {watcher->name_appeared_cv_mutex_};
    watcher->name_appeared_ = true;
    watcher->name_appeared_cv_.notify_all();
}

void bus_watcher::on_name_disappeared(GDBusConnection* /* connection */, const gchar* /* name */, gpointer user_data)
{
    bus_watcher* watcher = static_cast<bus_watcher*>(user_data);

    std::lock_guard lock {watcher->name_appeared_cv_mutex_};

    if (watcher->name_appeared_ && watcher->disappeared_handler_)
        watcher->disappeared_handler_();

    watcher->name_appeared_ = false;
}

} // end of namespace easydbuspp
