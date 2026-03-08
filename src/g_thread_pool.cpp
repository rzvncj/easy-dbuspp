// SPDX-FileCopyrightText: © 2024 Răzvan Cojocaru <razvanc@mailbox.org>
//
// SPDX-License-Identifier: AGPL-3.0-only

#include <g_thread_pool.h>
#include <stdexcept>
#include <types.h>

namespace easydbuspp {

g_thread_pool::g_thread_pool(GFunc func)
{
    GError* raw_error {nullptr};
    pool_ = g_thread_pool_new(func, nullptr, g_get_num_processors(), TRUE, &raw_error);
    g_error_ptr error {raw_error, g_error_free};

    if (!pool_)
        throw std::runtime_error("Could not create thread pool: " + std::string {error->message});
}

g_thread_pool::~g_thread_pool() noexcept
{
    g_thread_pool_free(pool_, TRUE, TRUE);
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
