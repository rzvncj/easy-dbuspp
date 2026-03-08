// SPDX-FileCopyrightText: © 2024 Răzvan Cojocaru <razvanc@mailbox.org>
//
// SPDX-License-Identifier: AGPL-3.0-only

#ifndef EASYDBUSPP_BUS_WATCHER_INL_INCLUDED
#define EASYDBUSPP_BUS_WATCHER_INL_INCLUDED

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

#endif // EASYDBUSPP_BUS_WATCHER_INL_INCLUDED
