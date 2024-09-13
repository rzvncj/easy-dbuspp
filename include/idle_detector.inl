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

#ifndef __IDLE_DETECTOR_INL_INCLUDED__
#define __IDLE_DETECTOR_INL_INCLUDED__

#include "main_loop.h"
#include <stdexcept>

namespace easydbuspp {

template <typename Rep, typename Period>
void idle_detector::start(const std::chrono::duration<Rep, Period>& timeout)
{
    if (idle_future_.valid())
        throw std::runtime_error("Idle detector already running!");

    stop_ = false;

    idle_future_ = std::async(std::launch::async, [this, timeout] {
        for (;;) {
            std::unique_lock lock {idle_cv_mutex_};

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
