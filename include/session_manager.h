// SPDX-FileCopyrightText: © 2024 Răzvan Cojocaru <razvanc@mailbox.org>
//
// SPDX-License-Identifier: AGPL-3.0-only

#ifndef __SESSION_MANAGER_H_INCLUDED__
#define __SESSION_MANAGER_H_INCLUDED__

#include "types.h"
#include <functional>
#include <gio/gio.h>
#include <memory>
#include <mutex>
#include <string>
#include <unordered_map>
#include <unordered_set>

namespace easydbuspp {

class object;
class proxy;

/*!
 * session_manager manages the D-Bus connection and runs (if asked to) the main loop, which makes sure
 * events such as method calls, property queries and signal deliveries are processed.
 */
class session_manager {

    using signal_handler_t = std::function<void(GVariant*)>;

public:
    /*!
     * Constructor. This synchronously connects to the D-Bus, but does not request a bus name.
     * It's the constructor to use when you only need to create proxies.
     *
     * @param bus_type Which bus to connect to (the session or system bus).
     */
    explicit session_manager(bus_type_t bus_type);

    /*!
     * Constructor. This connects to the D-Bus to request a bus name.
     * It's the constructor to use when you want to use objects (to provide a service).
     * You may use proxies with it as well.
     *
     * @param bus_type Which bus to connect to (the session or system bus).
     * @param bus_name The requested bus name.
     */
    session_manager(bus_type_t bus_type, const std::string& bus_name);

    //! Destructor. Detaches attached objects, releases the bus name.
    ~session_manager() noexcept;

    session_manager(const session_manager&)            = delete;
    session_manager& operator=(const session_manager&) = delete;

    //! Type for the name-lost handler callback.
    using name_lost_handler_t = std::function<void(const std::string& bus_name)>;

    /*!
     * Set a handler to be called when the bus name is lost. If no handler is set,
     * a `g_critical()` message will be logged and the main loop will be stopped.
     *
     * @param handler A callable that receives the lost bus name as a parameter.
     */
    void name_lost_handler(const name_lost_handler_t& handler);

    /*!
     * Register a callback to be called when a signal is received.
     *
     * @param name           The signal name.
     * @param callable       Any callable object (as long as it can be converted to an `std::function`).
     *                       So a lambda, a function pointer, a custom functor, etc. The parameter
     *                       types and number must match the types and number of arguments of the signal.
     * @param sender         (Optional) If this is not empty, the unique bus name of the sender of the
     *                       signal. It would only not be empty for unicast signals. Broadcast signals
     *                       do not require this.
     * @param interface_name (Optional) The interface name that defines the signal we're expecting.
     *                       Again, this will be missing for broadcast signals.
     * @param object_path    (Optional) The path of the object that is emitting the signal. Leave empty
     *                       when expecting broadcast signals.
     */
    template <typename C>
    void signal_subscribe(const std::string& signal_name, C&& callable, const std::string& sender = {},
                          const std::string& interface_name = {}, const object_path_t& object_path = {});

private:
    void attach(object* object_ptr);
    void detach(object* object_ptr);

    template <typename C, typename... A>
    signal_handler_t generate_signal_handler(C&& callable, const std::function<void(A...)>&);

    static void on_bus_acquired(GDBusConnection* connection, const gchar* name, gpointer user_data);
    static void on_name_acquired(GDBusConnection* connection, const gchar* name, gpointer user_data);
    static void on_name_lost(GDBusConnection* connection, const gchar* name, gpointer user_data);

    static void on_signal(GDBusConnection* connection, const gchar* sender_name, const gchar* object_path,
                          const gchar* interface_name, const gchar* signal_name, GVariant* parameters,
                          gpointer user_data);

private:
    guint                                             owner_id_ {0};
    std::string                                       bus_name_;
    std::unordered_set<object*>                       objects_;
    std::unordered_map<std::string, signal_handler_t> signal_handlers_;
    mutable std::mutex                                signal_handlers_mutex_;
    GDBusConnection*                                  connection_ {nullptr};
    name_lost_handler_t                               name_lost_handler_;
    std::mutex                                        name_lost_handler_mutex_;

    friend class object;
    friend class proxy;
};

} // end of namespace easydbuspp

#include "session_manager.inl"

#endif // __SESSION_MANAGER_H_INCLUDED__
