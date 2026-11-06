// SPDX-FileCopyrightText: © 2024 Răzvan Cojocaru <razvanc@mailbox.org>
//
// SPDX-License-Identifier: AGPL-3.0-only

#include <idle_detector.h>
#include <object.h>
#include <stdexcept>

namespace easydbuspp {

namespace {

void return_method_error(GDBusMethodInvocation* invocation, const gchar* interface_name, const char* message) noexcept
{
    const std::unique_ptr<gchar, decltype(&g_free)> error_name {g_strconcat(interface_name, ".MethodError", nullptr),
                                                                g_free};
    g_dbus_method_invocation_return_dbus_error(invocation, error_name.get(), message);
}

} // end of anonymous namespace

object::object(session_manager& session_mgr, const std::string& interface_name, const object_path_t& object_path)
    : session_manager_ {session_mgr}, manager_lifetime_ {session_mgr.lifetime_},
      registry_ {session_mgr.registry_}, interface_name_ {interface_name}, object_path_ {object_path},
      introspection_data_ {nullptr, g_dbus_node_info_unref}
{
    session_manager_.attach(this);

    if (session_manager_.connection_)
        g_warning(
            "Object '%s' was created after the bus connection was established, so it won't be registered "
            "until publish() is called",
            object_path_.generic_string().c_str());
}

// Must not be called from a method handler of the same object: it waits for in-flight requests to finish.
object::~object() noexcept
{
    {
        // If the session_manager is being destroyed (or is gone), it has cleared (or will clear) us out of the
        // registry under this lock, after disconnecting us. If we are still in it, the session_manager is alive, and
        // stays so until we release the lock.
        const std::lock_guard lock {registry_->mutex};

        if (registry_->objects.erase(this))
            disconnect();
    }

    lifetime_->close();
}

std::string object::introspection_xml() const
{
    return "<node>\n <interface name='" + xml_escape(interface_name_) + "'>\n" + methods_xml_ + properties_xml_
        + signals_xml_ + " </interface>\n</node>";
}

// Escapes a string so it can be safely embedded in the introspection XML.
std::string object::xml_escape(const std::string& text)
{
    const std::unique_ptr<gchar, decltype(&g_free)> escaped {g_markup_escape_text(text.c_str(), -1), g_free};
    return escaped.get();
}

std::string object::interface_name() const
{
    return interface_name_;
}

object_path_t object::object_path() const
{
    return object_path_;
}

void object::pre_request_handler(const pre_request_handler_t& handler)
{
    const std::lock_guard lock {pre_request_handler_mutex_};
    pre_request_handler_ = handler;
}

void object::connect()
{
    if (!session_manager_.connection_)
        throw std::runtime_error("Invalid input when attempting object connect");

    GError* raw_error {nullptr};

    introspection_data_ = g_dbus_node_info_ptr {g_dbus_node_info_new_for_xml(introspection_xml().c_str(), &raw_error),
                                                g_dbus_node_info_unref};
    g_error_ptr error {raw_error, g_error_free};

    if (!introspection_data_)
        throw std::runtime_error("Could not initialize introspection XML: " + std::string {error->message});

    // GDBus rejects invalid arguments without freeing the registration data, so check them first.
    if (!g_variant_is_object_path(object_path_.generic_string().c_str()))
        throw std::runtime_error("Could not register object: '" + object_path_.generic_string()
                                 + "' is not a valid object path");

    if (!g_dbus_is_interface_name(interface_name_.c_str()))
        throw std::runtime_error("Could not register object '" + object_path_.generic_string() + "': '"
                                 + interface_name_ + "' is not a valid interface name");

    // GDBus hands this to the request handlers instead of 'this', and keeps it alive until it has finished
    // dispatching to them. It frees it itself, also when the registration fails (the checks above cover the cases where it wouldn't).
    auto* registration_data = new lifetime_guard_ptr {lifetime_};

    raw_error = nullptr;

    registration_id_ = g_dbus_connection_register_object(
        session_manager_.connection_, object_path_.generic_string().c_str(), introspection_data_->interfaces[0],
        &interface_vtable_, registration_data, free_lifetime_guard_ptr, &raw_error);
    error.reset(raw_error);

    if (registration_id_ == 0)
        throw std::runtime_error("Could not register object '" + object_path_.generic_string()
                                 + "': " + (error ? std::string {error->message} : "unknown error"));
}

void object::publish()
{
    // Nothing to publish on if the session_manager is gone.
    if (!manager_lifetime_->enter<session_manager>())
        return;

    const request_scope scope {*manager_lifetime_};

    // Same lock as the bus-acquired callback, which connects this object too.
    const std::lock_guard lock {registry_->mutex};

    if (!session_manager_.connection_)
        return;

    disconnect();
    connect();
}

void object::disconnect()
{
    if (!session_manager_.connection_ || registration_id_ == 0)
        return;

    g_dbus_connection_unregister_object(session_manager_.connection_, registration_id_);
    registration_id_ = 0;
}

void object::handle_method_call(GDBusConnection* /* connection */, const gchar* sender, const gchar* object_path,
                                const gchar* interface_name, const gchar* method_name, GVariant* parameters,
                                GDBusMethodInvocation* invocation, gpointer user_data)
{
    using namespace std::string_literals;

    // Exceptions must not propagate into GLib, so everything up to the hand-off to the thread pool is guarded.
    try {
        const auto guard = *static_cast<lifetime_guard_ptr*>(user_data);

        // The request counts as in flight from now on, so the object can't be destroyed while it sits in the queue.
        object* obj_ptr = guard->enter<object>();

        if (!obj_ptr)
            throw std::runtime_error("The object no longer exists!");

        try {
            const dbus_context context {sender, interface_name, object_path, method_name};

            auto task = std::make_unique<std::function<void()>>([=] {
                const request_scope scope {*guard};

                try {
                    idle_detector::instance().ping(obj_ptr->object_path());

                    auto it = obj_ptr->methods_.find(method_name);

                    if (it == obj_ptr->methods_.end())
                        throw std::runtime_error("No method '"s + method_name + "' registered by object '"
                                                 + obj_ptr->object_path_.generic_string() + "'!");

                    {
                        pre_request_handler_t pre_handler;
                        {
                            const std::lock_guard lock {obj_ptr->pre_request_handler_mutex_};
                            pre_handler = obj_ptr->pre_request_handler_;
                        }
                        if (pre_handler)
                            pre_handler(request_type::METHOD, context);
                    }

                    GDBusMessage* message = g_dbus_method_invocation_get_message(invocation);
                    GUnixFDList*  fd_list = g_dbus_message_get_unix_fd_list(message);

                    auto [ret, out_fd_list] = it->second(parameters, fd_list, context);
                    g_dbus_method_invocation_return_value_with_unix_fd_list(invocation, ret, out_fd_list.get());

                } catch (const std::exception& e) {
                    return_method_error(invocation, interface_name, e.what());
                } catch (...) {
                    return_method_error(invocation, interface_name, "Unknown exception!");
                }
            });

            thread_pool().push(task.get());
            task.release();

        } catch (...) {
            guard->leave();
            throw;
        }

    } catch (const std::exception& e) {
        return_method_error(invocation, interface_name, e.what());
    } catch (...) {
        return_method_error(invocation, interface_name, "Unknown exception!");
    }
}

GVariant* object::handle_get_property(GDBusConnection* /* connection */, const gchar* sender, const gchar* object_path,
                                      const gchar* interface_name, const gchar* property_name, GError** error,
                                      gpointer user_data)
{
    using namespace std::string_literals;

    try {
        const auto guard   = *static_cast<lifetime_guard_ptr*>(user_data);
        object*    obj_ptr = guard->enter<object>();

        if (!obj_ptr)
            throw std::runtime_error("The object no longer exists!");

        const request_scope scope {*guard};

        idle_detector::instance().ping(obj_ptr->object_path());

        auto it = obj_ptr->properties_.find(property_name);

        if (it == obj_ptr->properties_.end())
            throw std::runtime_error("No property '"s + property_name + "' registered by object '"
                                     + obj_ptr->object_path_.generic_string() + "'!");

        auto&& [getter, setter] = it->second;

        if (!getter)
            throw std::runtime_error("Property '"s + property_name + "' for object '"
                                     + obj_ptr->object_path_.generic_string() + "' cannot be read!");

        const dbus_context context {sender, interface_name, object_path, property_name};

        {
            pre_request_handler_t pre_handler;
            {
                const std::lock_guard lock {obj_ptr->pre_request_handler_mutex_};
                pre_handler = obj_ptr->pre_request_handler_;
            }
            if (pre_handler)
                pre_handler(request_type::GET_PROPERTY, context);
        }

        return getter();

    } catch (const std::exception& e) {
        g_set_error(error, G_DBUS_ERROR, G_DBUS_ERROR_FAILED, "%s", e.what());
        return nullptr;
    } catch (...) {
        g_set_error_literal(error, G_DBUS_ERROR, G_DBUS_ERROR_FAILED, "Unknown exception!");
        return nullptr;
    }
}

gboolean object::handle_set_property(GDBusConnection* /* connection */, const gchar* sender, const gchar* object_path,
                                     const gchar* interface_name, const gchar* property_name, GVariant* value,
                                     GError** error, gpointer user_data)
{
    using namespace std::string_literals;

    try {
        const auto guard   = *static_cast<lifetime_guard_ptr*>(user_data);
        object*    obj_ptr = guard->enter<object>();

        if (!obj_ptr)
            throw std::runtime_error("The object no longer exists!");

        const request_scope scope {*guard};

        idle_detector::instance().ping(obj_ptr->object_path());

        auto it = obj_ptr->properties_.find(property_name);

        if (it == obj_ptr->properties_.end())
            throw std::runtime_error("No property '"s + property_name + "' registered by object '"
                                     + obj_ptr->object_path_.generic_string() + "'!");

        auto&& [getter, setter] = it->second;

        if (!setter)
            throw std::runtime_error("Property '"s + property_name + "' for object '"
                                     + obj_ptr->object_path_.generic_string() + "' is read only!");

        const dbus_context context {sender, interface_name, object_path, property_name};

        {
            pre_request_handler_t pre_handler;
            {
                const std::lock_guard lock {obj_ptr->pre_request_handler_mutex_};
                pre_handler = obj_ptr->pre_request_handler_;
            }
            if (pre_handler)
                pre_handler(request_type::SET_PROPERTY, context);
        }

        // GDBus aborts if a set_property handler returns FALSE without setting an error.
        if (!setter(value))
            throw std::runtime_error("Property '"s + property_name + "' for object '"
                                     + obj_ptr->object_path_.generic_string() + "' rejected the new value!");

        return TRUE;

    } catch (const std::exception& e) {
        g_set_error(error, G_DBUS_ERROR, G_DBUS_ERROR_FAILED, "%s", e.what());
        return FALSE;
    } catch (...) {
        g_set_error_literal(error, G_DBUS_ERROR, G_DBUS_ERROR_FAILED, "Unknown exception!");
        return FALSE;
    }
}

g_thread_pool& object::thread_pool()
{
    // Tasks still queued at exit run in the pool's destructor and use these singletons. Statics are destroyed in
    // reverse order of construction, so make sure these are constructed first and so outlive the pool.
    idle_detector::instance();
    main_loop::instance();

    static g_thread_pool pool {g_thread_pool_function};
    return pool;
}

void object::g_thread_pool_function(gpointer data, gpointer /* user_data */)
{
    using threadpool_fn_t = std::function<void()>;

    const std::unique_ptr<threadpool_fn_t> method_ptr {static_cast<threadpool_fn_t*>(data)};
    (*method_ptr)();
}

void object::emit_properties_update_signal(const std::string& property_name, GVariant* value) const
{
    if (!manager_lifetime_->enter<session_manager>()) {
        // Nobody is going to consume the floating reference.
        g_variant_ref_sink(value);
        g_variant_unref(value);
        return;
    }

    const request_scope scope {*manager_lifetime_};

    const g_variant_builder_ptr builder {g_variant_builder_new(G_VARIANT_TYPE_ARRAY), g_variant_builder_unref};
    g_variant_builder_add(builder.get(), "{sv}", property_name.c_str(), value);
    GVariant* property_update = g_variant_new("(sa{sv}as)", interface_name_.c_str(), builder.get(), nullptr);

    g_dbus_connection_emit_signal(session_manager_.connection_, nullptr, object_path_.generic_string().c_str(),
                                  "org.freedesktop.DBus.Properties", "PropertiesChanged", property_update, nullptr);
}

} // end of namespace easydbuspp
