// SPDX-FileCopyrightText: © 2024 Răzvan Cojocaru <rzvncj@gmail.com>
//
// SPDX-License-Identifier: AGPL-3.0-only

#include <g_thread_pool.h>
#include <stdexcept>

namespace easydbuspp {

g_thread_pool::g_thread_pool(GFunc func)
{
    pool_ = g_thread_pool_new(func, nullptr, g_get_num_processors(), TRUE, nullptr);
}

g_thread_pool::~g_thread_pool()
{
    g_thread_pool_free(pool_, TRUE, TRUE);
}

void g_thread_pool::push(gpointer data)
{
    GError* error {nullptr};

    if (!g_thread_pool_push(pool_, data, &error)) {
        std::string error_message = error->message;
        g_error_free(error);

        throw std::runtime_error("Could not push data to thread pool: " + error_message);
    }
}

} // end of namespace easydbuspp
