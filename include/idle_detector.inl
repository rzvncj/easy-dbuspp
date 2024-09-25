// SPDX-FileCopyrightText: © 2024 Răzvan Cojocaru <rzvncj@gmail.com>
//
// SPDX-License-Identifier: AGPL-3.0-only

#ifndef __IDLE_DETECTOR_INL_INCLUDED__
#define __IDLE_DETECTOR_INL_INCLUDED__

#include "main_loop.h"
#include <stdexcept>

namespace easydbuspp {

template <typename Rep, typename Period>
void idle_detector::enable(const std::chrono::duration<Rep, Period>& timeout)
{
    if (idle_future_.valid())
        throw std::runtime_error("Idle detector already running!");

    stop_ = false;

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

#endif // __IDLE_DETECTOR_INL_INCLUDED__
