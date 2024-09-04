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

#ifndef __OBJECT_H_INCLUDED__
#define __OBJECT_H_INCLUDED__

#include "g_thread_pool.h"
#include "params.h"
#include "type_mapping.h"
#include "types.h"
#include <cstdlib>
#include <functional>
#include <utility>

namespace easydbuspp {

class session_manager;

/*!
 * This is the basic building block for a D-Bus service. It's a D-Bus interface and object all rolled into one.
 * When its associated session_manager object gets a bus connection, it will register the object on the bus and
 * start using the registered method callbacks and properties as incoming requests require.
 */
class object {

    using method_handler_t         = std::function<GVariant*(GVariant*)>;
    using property_read_handler_t  = std::function<GVariant*()>;
    using property_write_handler_t = std::function<gboolean(GVariant*)>;

public:
    //! Used in (optional) pre-request handlers.
    enum class request_type { METHOD, GET_PROPERTY, SET_PROPERTY };

    //! Type to which all pre-request handler callbacks must conform.
    using pre_request_handler_t = std::function<void(request_type, const std::string&, const std::string&)>;

public:
    /*!
     * Constructor.
     *
     * @param session_mgr     This object is responsible for establishing and maintaining the D-Bus connection.
     * @param interface_name  The name of the interface that will get registered on the bus once the
     *                        session_manager connection is up. These things look like a reverse-domain, e.g.
     *                        "net.my_domain.my_host.widget_interface".
     * @param object_path     The object path that will uniquely identify this object. It would look like a
     *                        UNIX filesystem path (e.g. "/net/my_domain/my_host/shiniest_widget").
     */
    object(session_manager& session_mgr, const std::string& interface_name, const object_path_t& object_path);

    //! Destructor. If applicable, will disconnect the object from its D-Bus connection.
    ~object();

    object(const object&)            = delete;
    object& operator=(const object&) = delete;

    /*!
     * Associate a callback with a D-Bus method (generates XML introspection data as well).
     *
     * @param name               The name of the method, as displayed when introspecting the D-Bus object.
     * @param callable           Any callable object at all (it can be an `std::function`, a lambda, a
     *                           function pointer, a custom functor, etc.). As long as it can be converted
     *                           to a corresponding `std::function`.
     * @param in_argument_names  (Optional) A list of parameter names that match each of the arguments of
     *                           `callable`. If the number of names differs from the number of parameters
     *                           `callable` takes, you will get an exception thrown.
     *                           If this parameter is missing, unique parameter names will automatically be
     *                           generated.
     * @param out_argument_names (Optional) A list of names that match each of the member of the return tuple
     *                           of `callable` (which is how we return several values). If `callable` returns
     *                           a single value, this vector will have to contain a single argument. Similarly
     *                           to `in_argument_names`, if this is not specified unique output argument names
     *                           will be generated automatically, and if the size of this vector differs from
     *                           the number of returned arguments, you will get an exception thrown at you.
     * @throw                    std::runtime_error
     */
    template <typename C>
    void add_method(const std::string& name, C&& callable, const std::vector<std::string>& in_argument_names = {},
                    const std::vector<std::string>& out_argument_names = {});

    /*!
     * Add a property (generates XML introspection data as well).
     *
     * @param name  The name of the property, as displayed when introspecting the D-Bus object.
     * @param value Any library supported value at all. The machinery behind this function will automatically
     *              detect and decide whether this will be a read-only or a readwrite property, based on
     *              whether it is a C++ lvalue or not.
     */
    template <typename T>
    void add_property(const std::string& name, T&& value);

    /*!
     * Add a property (generates XML introspection data as well). This will create a read, readwrite,
     * or write-only property depending on which callback is being passed in. Default (uninitialized)
     * functors mean that that particular access type to the property is disabled.
     *
     * @param name   The name of the property, as displayed when introspecting the D-Bus object.
     * @param getter A getter function (i.e. anything taking no arguments and returning a `T`). This
     *               will be used to retrieve the value of the property on each property query.
     * @param setter A setter function (i.e. anything taking a `const T&` argument and returning a
     *               `bool` signifying success/failure to set). This will be called when a D-Bus client
     *               will try to set the variable.
     */
    template <typename T>
    void add_property(const std::string& name, const std::function<T()>& getter,
                      const std::function<bool(const T&)>& setter);

    /*!
     * Add a broadcast signal (generates XML introspection data as well). Broadcast signals are sent to
     * everybody, on all buses. A signal may have parameters, so when you receive one you may also get
     * additional data with it. This is the "send a signal out part".
     *
     * @param name           The name of the signal, as displayed when introspecting the D-Bus object.
     * @param argument_names (Optional) If this is not empty (which is the default), its values will
     *                       be used to name the signal's parameters (denoted by the template
     *                       arguments). If it is empty, we'll just generate unique names for the
     *                       signal parameters.
     * @return               A function you can now call to emit the signal. Just call it as you would
     *                       any regular function.
     * @throw                std::runtime_error
     */
    template <typename... A>
    std::function<void(A...)> add_broadcast_signal(const std::string&              name,
                                                   const std::vector<std::string>& argument_names = {});

    /*!
     * Add an unicast signal (generates XML introspection data as well). Unicast signals are finicky
     * and not used that often, but they're here for completeness. To use them properly, the sender
     * must explicitly specify the destination bus, and the recipient must explicitly specify that it
     * waits for a signal coming from the sender's bus. No match, no signal.
     * A signal may have parameters, so when you receive one you may also get additional data with it.
     * This is the "send a signal out part".
     *
     * @param name           The name of the signal, as displayed when introspecting the D-Bus object.
     * @param argument_names (Optional) If this is not empty (which is the default), its values will
     *                       be used to name the signal's parameters (denoted by the template
     *                       arguments). If it is empty, we'll just generate unique names for the
     *                       signal parameters.
     * @return               A function you can now call to emit the signal. Just call it as you would
     *                       any regular function. Please note that unlike `add_broadcast_signal()`,
     *                       the `std::function` returned here has an additional `std::string` parameter.
     *                       This is the destination bus name. So where for a broadcast signal you'd
     *                       just say `my_signal(parameters)` and it would be sent to everyone, here
     *                       you'd need to say `my_signal(<unique_bus_name>, parameters)`.
     * @throw                std::runtime_error
     */
    template <typename... A>
    std::function<void(const std::string&, A...)>
    add_unicast_signal(const std::string& name, const std::vector<std::string>& argument_names = {});

    /*!
     * Add a pre-request handler function. This function (if added) will run before a method call, or
     * a property set or get operation. The parameters to the function will be: request type, sender
     * (as in the bus name of the sender), and name (method name for methods, property name for
     * properties). If processing the current method or property needs to be denied, throw an
     * std::exception-derived exception from the handler.
     *
     * @param handler Callback to invoke before any method call, setting or getting a property value.
     */
    void pre_request_handler(const pre_request_handler_t& handler);

    //! Returns this object's interface name.
    std::string interface_name() const;

    //! Returns this object's unique object path.
    object_path_t object_path() const;

private:
    std::string introspection_xml() const;

    void connect();
    void disconnect();

    template <typename C, typename R, typename... A>
    method_handler_t add_method_helper(const std::string& name, C&& callable, const std::function<R(A...)>&,
                                       const std::vector<std::string>& in_argument_names,
                                       const std::vector<std::string>& out_argument_names);

    template <typename... A>
    std::function<void(const std::string&, A...)> add_signal(const std::string& name, bool unicast,
                                                             const std::vector<std::string>& argument_names);

    void emit_properties_update_signal(const std::string& property_name, GVariant* value) const;

    static void handle_method_call(GDBusConnection* connection, const gchar* sender, const gchar* object_path,
                                   const gchar* interface_name, const gchar* method_name, GVariant* parameters,
                                   GDBusMethodInvocation* invocation, gpointer user_data);

    static GVariant* handle_get_property(GDBusConnection* connection, const gchar* sender, const gchar* object_path,
                                         const gchar* interface_name, const gchar* property_name, GError** error,
                                         gpointer user_data);

    static gboolean handle_set_property(GDBusConnection* connection, const gchar* sender, const gchar* object_path,
                                        const gchar* interface_name, const gchar* property_name, GVariant* value,
                                        GError** error, gpointer user_data);

    static void g_thread_pool_function(gpointer data, gpointer user_data);

private:
    session_manager&                                                                              session_manager_;
    guint                                                                                         registration_id_ {0};
    std::string                                                                                   interface_name_;
    object_path_t                                                                                 object_path_;
    std::string                                                                                   methods_xml_;
    std::string                                                                                   properties_xml_;
    std::string                                                                                   signals_xml_;
    std::unordered_map<std::string, method_handler_t>                                             methods_;
    std::unordered_map<std::string, std::pair<property_read_handler_t, property_write_handler_t>> properties_;
    g_dbus_node_info_ptr                                                                          introspection_data_;
    static g_thread_pool                                                                          thread_pool_;
    pre_request_handler_t                                                                         pre_request_handler_;
    static inline const GDBusInterfaceVTable                                                      interface_vtable_ {
        handle_method_call, handle_get_property, handle_set_property, {}};

    friend class session_manager;
};

} // end of namespace easydbuspp

#include "object.inl"

#endif // __OBJECT_H_INCLUDED__
