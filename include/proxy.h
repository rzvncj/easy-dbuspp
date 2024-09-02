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

#ifndef __PROXY_H_INCLUDED__
#define __PROXY_H_INCLUDED__

#include "params.h"
#include "type_mapping.h"
#include "types.h"
#include <cstdlib>
#include <functional>
#include <utility>

namespace easydbuspp {

class session_manager;

class proxy {

public:
    proxy(session_manager& session_manager, const std::string& bus_name, const std::string& interface_name,
          const object_path_t& object_path);
    virtual ~proxy();

    proxy(const proxy&)            = delete;
    proxy& operator=(const proxy&) = delete;

    std::string unique_bus_name() const;

    template <typename R, typename... A>
    R call(const std::string& method_name, A... parameters) const;

    template <typename T>
    T cached_property(const std::string& property_name) const;

    template <typename T>
    void cached_property(const std::string& property_name, const T& new_value);

    // This will actually incur a proxy call to org.freedesktop.DBus.Properties.Set.
    template <typename T>
    void property(const std::string& property_name, const T& new_value);

private:
    session_manager& session_manager_;
    std::string      bus_name_;
    std::string      interface_name_;
    object_path_t    object_path_;
    GDBusProxy*      proxy_ {nullptr};
};

} // end of namespace easydbuspp

#include "proxy.inl"

#endif // __PROXY_H_INCLUDED__
