// SPDX-FileCopyrightText: © 2024 Răzvan Cojocaru <rzvncj@gmail.com>
//
// SPDX-License-Identifier: AGPL-3.0-only

#ifndef __G_THREAD_POOL_H_INCLUDED__
#define __G_THREAD_POOL_H_INCLUDED__

#include <glib.h>

namespace easydbuspp {

class g_thread_pool {

public:
    explicit g_thread_pool(GFunc func);
    ~g_thread_pool();

    g_thread_pool(const g_thread_pool&)            = delete;
    g_thread_pool& operator=(const g_thread_pool&) = delete;

    void push(gpointer data);

private:
    GThreadPool* pool_ {nullptr};
};

} // end of namespace easydbuspp

#endif // __G_THREAD_POOL_H_INCLUDED__
