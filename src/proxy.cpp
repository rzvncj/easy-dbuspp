// SPDX-FileCopyrightText: © 2024 Răzvan Cojocaru <razvanc@mailbox.org>
//
// SPDX-License-Identifier: AGPL-3.0-only

#include <proxy.h>
#include <session_manager.h>
#include <stdexcept>

namespace easydbuspp {

proxy::proxy(session_manager& session_mgr, const std::string& bus_name, const std::string& interface_name,
             const object_path_t& object_path)
    : session_manager_ {session_mgr}, bus_name_ {bus_name}, interface_name_ {interface_name}, object_path_ {object_path}
{
    if (!session_manager_.connection_)
        throw std::runtime_error("Could not create proxy: no live D-Bus connection!");

    GError* raw_error {nullptr};

    proxy_ = g_dbus_proxy_new_sync(session_manager_.connection_, G_DBUS_PROXY_FLAGS_NONE,
                                   nullptr /* GDBusInterfaceInfo */, bus_name.c_str(),
                                   object_path.generic_string().c_str(), interface_name.c_str(), nullptr, &raw_error);
    g_error_ptr error {raw_error, g_error_free};

    if (!proxy_)
        throw std::runtime_error("Could not create proxy: " + std::string {error->message});
}

proxy::~proxy() noexcept
{
    if (proxy_)
        g_object_unref(proxy_);
}

std::string proxy::unique_bus_name() const
{
    return g_dbus_connection_get_unique_name(g_dbus_proxy_get_connection(proxy_));
}

} // end of namespace easydbuspp
