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

#ifndef __MAIN_LOOP_H_INCLUDED__
#define __MAIN_LOOP_H_INCLUDED__

#include <future>
#include <gio/gio.h>

namespace easydbuspp {

/*!
 * Wrapper for a glib2 main loop. Only one of these should run at one time, hence the Meyers singleton.
 */
class main_loop {

private:
    //! Constructor. Private, to make sure we only ever have one instance.
    main_loop();

    main_loop(const main_loop&)            = delete;
    main_loop& operator=(const main_loop&) = delete;

public:
    ~main_loop();

    //! Start the D-Bus event processing loop.
    void run();

    //! Start the D-Bus event processing loop asynchronously.
    void run_async();

    //! Wait on an async run to finish.
    void wait();

    //! Stop the D-Bus event processing loop.
    void stop();

    //! Return the unique, per-process instance.
    static main_loop& instance();

private:
    static int stop_sighandler(void* param);

private:
    bool              destructor_called_ {false};
    GMainLoop*        loop_ {nullptr};
    std::future<void> run_future_;
};

} // end of namespace easydbuspp

#endif // __MAIN_LOOP_H_INCLUDED__
