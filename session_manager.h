#ifndef __SESSION_MANAGER_H_INCLUDED__
#define __SESSION_MANAGER_H_INCLUDED__

#include "dbus_object.h"
#include <gio/gio.h>
#include <memory>
#include <string>
#include <unordered_map>

class dbus_session_manager {

public:
    dbus_session_manager(GBusType bus_type, const std::string& bus_name);
    ~dbus_session_manager();

    void manage(std::shared_ptr<dbus_object> object_ptr);

    void run();

private:
    static void on_bus_acquired(GDBusConnection* connection, const gchar* name, gpointer user_data);

    static void handle_method_call(GDBusConnection* connection, const gchar* sender, const gchar* object_path,
                                   const gchar* interface_name, const gchar* method_name, GVariant* parameters,
                                   GDBusMethodInvocation* invocation, gpointer user_data);

    static void on_name_acquired(GDBusConnection* connection, const gchar* name, gpointer user_data);

    static void on_name_lost(GDBusConnection* connection, const gchar* name, gpointer user_data);

    static GVariant* handle_get_property(GDBusConnection* connection, const gchar* sender, const gchar* object_path,
                                         const gchar* interface_name, const gchar* property_name, GError** error,
                                         gpointer user_data);

    static gboolean handle_set_property(GDBusConnection* connection, const gchar* sender, const gchar* object_path,
                                        const gchar* interface_name, const gchar* property_name, GVariant* value,
                                        GError** error, gpointer user_data);

private:
    guint                                                         owner_id_;
    std::unordered_map<std::string, std::shared_ptr<dbus_object>> managed_objects_;
    static inline GDBusNodeInfo*                                  introspection_data_ {nullptr};
    static inline const GDBusInterfaceVTable                      interface_vtable_ {
        handle_method_call, handle_get_property, handle_set_property, {}};
    static inline GMainLoop* loop_ {nullptr};
};

#endif // __SESSION_MANAGER_H_INCLUDED__
