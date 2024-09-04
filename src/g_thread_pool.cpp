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
