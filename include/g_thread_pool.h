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
