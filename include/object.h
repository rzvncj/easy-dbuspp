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

#include "params.h"
#include "type_mapping.h"
#include "types.h"
#include <asio.hpp>
#include <cstdlib>
#include <functional>
#include <utility>

namespace easydbuspp {

class session_manager;

class object {

    using method_handler_t         = std::function<GVariant*(GVariant*)>;
    using property_read_handler_t  = std::function<GVariant*()>;
    using property_write_handler_t = std::function<gboolean(GVariant*)>;

public:
    object(session_manager& session_manager, const std::string& interface_name, const object_path_t& object_path);
    ~object();

    object(const object&)            = delete;
    object& operator=(const object&) = delete;

    template <typename C>
    void add_method(const std::string& name, C&& callable, const std::vector<std::string>& in_argument_names = {},
                    const std::vector<std::string>& out_argument_names = {});

    template <typename T>
    void add_property(const std::string& name, T&& value);

    template <typename T>
    void add_property(const std::string& name, std::function<T()> getter, std::function<bool(const T&)> setter);

    template <typename... A>
    std::function<void(A...)> add_broadcast_signal(const std::string&              name,
                                                   const std::vector<std::string>& argument_names = {});

    template <typename... A>
    std::function<void(const std::string&, A...)>
    add_unicast_signal(const std::string& name, const std::vector<std::string>& argument_names = {});

    std::string   interface_name() const;
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
    static inline asio::thread_pool                                                               thread_pool_;
    static inline const GDBusInterfaceVTable                                                      interface_vtable_ {
        handle_method_call, handle_get_property, handle_set_property, {}};

    friend class session_manager;
};

} // end of namespace easydbuspp

#include "object.inl"

#endif // __OBJECT_H_INCLUDED__
