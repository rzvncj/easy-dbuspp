// SPDX-FileCopyrightText: © 2024 Răzvan Cojocaru <razvanc@mailbox.org>
//
// SPDX-License-Identifier: AGPL-3.0-only

#ifndef EASYDBUSPP_IDLE_DETECTOR_INL_INCLUDED
#define EASYDBUSPP_IDLE_DETECTOR_INL_INCLUDED

#include "main_loop.h"
#include <stdexcept>

namespace easydbuspp {

template <typename Rep, typename Period>
void idle_detector::enable(const std::chrono::duration<Rep, Period>& timeout)
{
    if (idle_future_.valid()) {
        // A detector whose timeout has fired has finished its thread; reap it so it can be enabled again.
        if (idle_future_.wait_for(std::chrono::seconds::zero()) != std::future_status::ready)
            throw std::runtime_error("Idle detector already running!");

        idle_future_.get();
    }

    {
        const std::lock_guard lock {idle_mutex_};
        stop_         = false;
        enabled_      = true;
        request_ping_ = false;
    }

    idle_future_ = std::async(std::launch::async, [this, timeout] {
        for (;;) {
            std::unique_lock lock {idle_mutex_};

            if (!idle_cv_.wait_for(lock, timeout, [this] {
                    return request_ping_ || stop_;
                })) {
                main_loop::instance().stop();
                return;
            } else {
                request_ping_ = false;

                if (stop_)
                    return;
            }
        }
    });
}

} // end of namespace easydbuspp

#endif // EASYDBUSPP_IDLE_DETECTOR_INL_INCLUDED
