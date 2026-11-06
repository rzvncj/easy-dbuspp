// SPDX-FileCopyrightText: © 2024 Răzvan Cojocaru <razvanc@mailbox.org>
//
// SPDX-License-Identifier: AGPL-3.0-only

#include <glib-unix.h>
#include <main_loop.h>

namespace easydbuspp {

main_loop::main_loop() : loop_ {g_main_loop_new(nullptr, FALSE)}
{
    g_unix_signal_add(SIGINT, stop_sighandler, loop_);
    g_unix_signal_add(SIGTERM, stop_sighandler, loop_);
}

main_loop::~main_loop() noexcept
{
    if (g_main_loop_is_running(loop_))
        g_main_loop_quit(loop_);

    g_main_loop_unref(loop_);
}

main_loop& main_loop::instance()
{
    static main_loop the_instance;
    return the_instance;
}

void main_loop::run() noexcept
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

void main_loop::stop() noexcept
{
    // g_main_loop_quit() is a no-op if the loop isn't running yet, which would lose a stop() that races with
    // run_async(). A source queued on the loop's context isn't lost: it fires as soon as the loop runs.
    if (stop_pending_.exchange(true))
        return;

    g_idle_add(
        [](gpointer param) -> gboolean {
            auto* self = static_cast<main_loop*>(param);

            self->stop_pending_ = false;
            g_main_loop_quit(self->loop_);

            return G_SOURCE_REMOVE;
        },
        this);
}

int main_loop::stop_sighandler(void* param)
{
    auto* loop = static_cast<GMainLoop*>(param);

    if (g_main_loop_is_running(loop))
        g_main_loop_quit(loop);

    return G_SOURCE_CONTINUE;
}

} // end of namespace easydbuspp
