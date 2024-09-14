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

#include <idle_detector.h>
#include <object.h>

namespace easydbuspp {

idle_detector::~idle_detector()
{
    // TODO: I'm betting that the future::get() call in disable() won't throw.
    disable();
}

idle_detector& idle_detector::instance()
{
    static idle_detector the_instance;
    return the_instance;
}

void idle_detector::disable()
{
    if (!idle_future_.valid())
        return;

    {
        std::lock_guard lock(idle_mutex_);
        stop_ = true;
    }

    idle_cv_.notify_all();

    if (idle_future_.valid())
        idle_future_.get();
}

void idle_detector::ping(const object_path_t& object_path)
{
    if (!idle_future_.valid())
        return;

    {
        std::lock_guard lock(idle_mutex_);

        if (excluded_objects_.find(object_path) != excluded_objects_.end())
            return;

        request_ping_ = true;
    }

    idle_cv_.notify_all();
}

void idle_detector::exclude(const object& obj)
{
    std::lock_guard lock(idle_mutex_);

    excluded_objects_.insert(obj.object_path());
}

} // end of namespace easydbuspp
