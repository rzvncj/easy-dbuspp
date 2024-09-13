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

namespace easydbuspp {

idle_detector::~idle_detector()
{
    // TODO: I'm betting that the future::get() call in stop() won't throw.
    stop();
}

idle_detector& idle_detector::instance()
{
    static idle_detector the_instance;
    return the_instance;
}

void idle_detector::stop()
{
    if (!idle_future_.valid())
        return;

    {
        std::lock_guard lock(idle_cv_mutex_);
        stop_ = true;
    }

    idle_cv_.notify_all();

    if (idle_future_.valid())
        idle_future_.get();
}

void idle_detector::ping()
{
    if (!idle_future_.valid())
        return;

    {
        std::lock_guard lock(idle_cv_mutex_);
        request_ping_ = true;
    }

    idle_cv_.notify_all();
}

} // end of namespace easydbuspp
