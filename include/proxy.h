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

/*!
 * Proxies connect to D-Bus objects, call their methods, examine and set their properties.
 */
class proxy {

public:
    /*!
     * Constructor.
     *
     * @param session_mgr    The session_manager object that manages an established connection to
     *                       the D-Bus.
     * @param bus_name       The bus name that the remote object is connected to.
     * @param interface_name The interface we want to use on the remote object.
     * @param object_path    The path of the remote object.
     */
    proxy(session_manager& session_mgr, const std::string& bus_name, const std::string& interface_name,
          const object_path_t& object_path);

    //! Destructor. Cleans managed resources up.
    virtual ~proxy();

    proxy(const proxy&)            = delete;
    proxy& operator=(const proxy&) = delete;

    /*!
     * Returns the bus name that the proxy is connected to (NOT the bus name of the remote object
     * it represents!).
     */
    std::string unique_bus_name() const;

    /*!
     * Calls a method on the remote object.
     *
     * @param method_name The name of the method we want to call on the remote object.
     * @param parameters  Any parameters that the remote method takes. The types and number of
     *                    parameters passed here must match the types and number of arguments
     *                    of the method.
     * @return            Whatever the remote method returns. For multiple return values, this
     *                    will need to be an `std::tuple`.
     * @throw             std::runtime_error
     */
    template <typename R, typename... A>
    R call(const std::string& method_name, A... parameters) const;

    /*!
     * Returns a cached property. When we initialize the proxy, it will cache all the properties
     * of the remote object. When one of those are changed, assuming that the remote object
     * triggers the `"PropertiesChanged"` signal, our cache gets updated as well.
     *
     * @param  property_name The name of the property we want to query.
     * @return The value of the requested property.
     */
    template <typename T>
    T cached_property(const std::string& property_name) const;

    /*!
     * Sets a cached property. This will only affect the cache of our proxy, not the remote
     * object. If changing the property on the actual remote object is desired, use the
     * `property()` member function instead.
     *
     * @param property_name The name of the property we want to query.
     * @param new_value     The new value we want to set the property to.
     */
    template <typename T>
    void cached_property(const std::string& property_name, const T& new_value);

    /*!
     * Sets a property directly on the remote object. Assuming that the remote object triggers
     * the `"PropertiesChanged"` signal, our cache gets updated as well. So a subsequent
     * read from the cache will reflect the remote change.
     *
     * This is expensive, because it incurs a proxy call to `org.freedesktop.DBus.Properties.Set`.
     *
     * @param property_name The name of the property we want to query.
     * @param new_value     The new value we want to set the property to.
     */
    template <typename T>
    void property(const std::string& property_name, const T& new_value);

    /*!
     * Gets a property directly from the remote-object (not from the cache).
     *
     * This is expensive, because it incurs a proxy call to `org.freedesktop.DBus.Properties.Get`.
     *
     * @param property_name The name of the property we want to query.
     * @return The value of the requested property.
     */
    template <typename T>
    T property(const std::string& property_name) const;

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
