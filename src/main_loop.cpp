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

#include <glib-unix.h>
#include <main_loop.h>
#include <stdexcept>

#include <iostream>

namespace easydbuspp {

main_loop::main_loop()
{
    loop_ = g_main_loop_new(nullptr, FALSE);

    g_unix_signal_add(SIGINT, stop_sighandler, loop_);
    g_unix_signal_add(SIGTERM, stop_sighandler, loop_);
}

main_loop::~main_loop()
{
    stop();
    g_main_loop_unref(loop_);
}

main_loop& main_loop::instance()
{
    static main_loop the_instance;
    return the_instance;
}

void main_loop::run()
{
    if (!g_main_loop_is_running(loop_))
        g_main_loop_run(loop_);
}

void main_loop::run_async()
{
    if (run_future_.valid())
        return;

    run_future_ = std::async(std::launch::async, [this] {
        run();
    });
}

void main_loop::wait()
{
    if (run_future_.valid())
        run_future_.get();
}

void main_loop::stop()
{
    if (g_main_loop_is_running(loop_))
        g_main_loop_quit(loop_);
}

int main_loop::stop_sighandler(void* param)
{
    GMainLoop* loop = static_cast<GMainLoop*>(param);

    if (g_main_loop_is_running(loop))
        g_main_loop_quit(loop);

    return G_SOURCE_CONTINUE;
}

} // end of namespace easydbuspp
