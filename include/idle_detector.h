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

#ifndef __IDLE_DETECTOR_H_INCLUDED__
#define __IDLE_DETECTOR_H_INCLUDED__

#include <chrono>
#include <condition_variable>
#include <future>
#include <mutex>

namespace easydbuspp {

/*!
 * When no requests have come to the managed object in a specified timeframe,
 * this class will shut the main loop down if it is running.
 */
class idle_detector {

private:
    //! Constructor. Private, to make sure we only ever have one instance.
    idle_detector() = default;

public:
    //! Destructor. Will stop the idle detector thread, if running.
    ~idle_detector();

    idle_detector(const idle_detector&)            = delete;
    idle_detector& operator=(const idle_detector&) = delete;

    /*!
     * Start the idle detector loop in a new thread. Can be stopped with `stop()`.
     *
     * @param timeout If this timeout is reached with no requests (method calls, property reads or writes)
     *                after calling this function, the application will exit. If the idle detector thread
     *                is already running, the function will throw an `std::runtime_error` exception.
     * @throw         std::runtime_error
     */
    template <typename Rep, typename Period>
    void start(const std::chrono::duration<Rep, Period>& timeout);

    //! Stops the idle detector thread, if it's running.
    void stop();

    //! Resets the timeout. Every time this function gets called, the timer resets to timeout again.
    void ping();

public:
    //! Return the unique, per-process instance.
    static idle_detector& instance();

private:
    std::condition_variable idle_cv_;
    std::mutex              idle_cv_mutex_;
    bool                    request_ping_ {false};
    bool                    stop_ {false};
    std::future<void>       idle_future_;
};

} // end of namespace easydbuspp

#include "idle_detector.inl"

#endif // __IDLE_DETECTOR_H_INCLUDED__
