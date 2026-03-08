// SPDX-FileCopyrightText: © 2024 Răzvan Cojocaru <razvanc@mailbox.org>
//
// SPDX-License-Identifier: AGPL-3.0-only

#ifndef EASYDBUSPP_G_THREAD_POOL_H_INCLUDED
#define EASYDBUSPP_G_THREAD_POOL_H_INCLUDED

#include <glib.h>

namespace easydbuspp {

class g_thread_pool {

public:
    explicit g_thread_pool(GFunc func);
    ~g_thread_pool() noexcept;

    g_thread_pool(const g_thread_pool&)            = delete;
    g_thread_pool& operator=(const g_thread_pool&) = delete;

    void push(gpointer data);

private:
    GThreadPool* pool_ {nullptr};
};

} // end of namespace easydbuspp

#endif // EASYDBUSPP_G_THREAD_POOL_H_INCLUDED
