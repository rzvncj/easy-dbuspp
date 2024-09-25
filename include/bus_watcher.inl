// SPDX-FileCopyrightText: © 2024 Răzvan Cojocaru <rzvncj@gmail.com>
//
// SPDX-License-Identifier: AGPL-3.0-only

#ifndef __BUS_WATCHER_INL_INCLUDED__
#define __BUS_WATCHER_INL_INCLUDED__

#include <stdexcept>

namespace easydbuspp {

template <typename Rep, typename Period>
void bus_watcher::wait_for(const std::chrono::duration<Rep, Period>& timeout)
{
    std::unique_lock<std::mutex> lock {name_appeared_cv_mutex_};

    if (!name_appeared_cv_.wait_for(lock, timeout, [this] {
            return name_appeared_;
        }))
        throw std::runtime_error("Timeout before bus name appeared!");
}

} // end of namespace easydbuspp

#endif // __BUS_WATCHER_INL_INCLUDED__
