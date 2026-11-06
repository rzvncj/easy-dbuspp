// SPDX-FileCopyrightText: © 2024 Răzvan Cojocaru <razvanc@mailbox.org>
//
// SPDX-License-Identifier: AGPL-3.0-only

#include <algorithm>
#include <g_thread_pool.h>
#include <stdexcept>
#include <types.h>

namespace easydbuspp {

g_thread_pool::g_thread_pool(GFunc func)
{
    GError* raw_error {nullptr};

    // Not exclusive: threads are started on demand (and reaped when idle) instead of all of them up front.
    // At least 4 threads, so a few methods can wait on synchronous calls to other objects of this process (which
    // need threads of their own) even on a machine with a single CPU.
    pool_ = g_thread_pool_new(func, nullptr, std::max<gint>(4, g_get_num_processors()), FALSE, &raw_error);
    g_error_ptr error {raw_error, g_error_free};

    if (!pool_)
        throw std::runtime_error("Could not create thread pool: " + std::string {error->message});
}

g_thread_pool::~g_thread_pool() noexcept
{
    // Not immediate: queued tasks still have to run, or their requests would never be answered or released.
    g_thread_pool_free(pool_, FALSE, TRUE);
}

void g_thread_pool::push(gpointer data)
{
    GError* raw_error {nullptr};

    if (!g_thread_pool_push(pool_, data, &raw_error)) {
        g_error_ptr error {raw_error, g_error_free};
        throw std::runtime_error("Could not push data to thread pool: " + std::string {error->message});
    }
}

} // end of namespace easydbuspp
