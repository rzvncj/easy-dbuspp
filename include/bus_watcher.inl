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

#ifndef __BUS_WATCHER_INL_INCLUDED__
#define __BUS_WATCHER_INL_INCLUDED__

#include <future>

namespace easydbuspp {

template <typename Rep, typename Period>
void bus_watcher::wait_for(const std::chrono::duration<Rep, Period>& timeout)
{
    auto a = std::async(std::launch::async, [this] {
        g_main_loop_run(loop_);
    });

    a.wait_for(timeout);

    if (g_main_loop_is_running(loop_))
        g_main_loop_quit(loop_);

    if (a.valid())
        a.get();
}

} // end of namespace easydbuspp

#endif // __BUS_WATCHER_INL_INCLUDED__
