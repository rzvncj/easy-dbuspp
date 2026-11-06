// SPDX-FileCopyrightText: © 2024 Răzvan Cojocaru <razvanc@mailbox.org>
//
// SPDX-License-Identifier: AGPL-3.0-only

#include "wait_for_service.h"
#include <atomic>
#include <easydbuspp.h>
#include <iostream>

namespace {

std::atomic<int> registration_failures {0};
std::atomic<int> late_object_warnings {0};

GLogWriterOutput capture_log(GLogLevelFlags level, const GLogField* fields, gsize n_fields, gpointer user_data)
{
    for (gsize i = 0; i < n_fields; ++i) {
        if (std::string_view {fields[i].key} != "MESSAGE")
            continue;

        const std::string_view message {static_cast<const char*>(fields[i].value)};

        if (message.find("Could not register object") != std::string_view::npos)
            ++registration_failures;
        else if (message.find("was created after the bus connection") != std::string_view::npos)
            ++late_object_warnings;
    }

    return g_log_writer_default(level, fields, n_fields, user_data);
}

} // end of anonymous namespace

int main()
{
    try {
        g_log_set_writer_func(capture_log, nullptr, nullptr);

        const std::string               BUS_NAME {"net.test.EasyDBuspp.Test"};
        const std::string               INTERFACE_NAME {"net.test.EasyDBuspp.TestInterface"};
        const easydbuspp::object_path_t OBJECT_PATH {"/net/test/EasyDBuspp/TestObject"};
        const easydbuspp::object_path_t LATE_OBJECT_PATH {"/net/test/EasyDBuspp/LateObject"};

        easydbuspp::session_manager obj_session_manager {easydbuspp::bus_type_t::SESSION, BUS_NAME};
        easydbuspp::object          object {obj_session_manager, INTERFACE_NAME, OBJECT_PATH};

        object.add_method("Ping", [] {
            return std::string {"pong"};
        });

        // Same path and interface as 'object': GDBus refuses one of the two registrations, and we must say so.
        easydbuspp::object duplicate {obj_session_manager, INTERFACE_NAME, OBJECT_PATH};

        duplicate.add_method("Ping", [] {
            return std::string {"pong"};
        });

        easydbuspp::main_loop::instance().run_async();
        wait_for_service(BUS_NAME);

        if (registration_failures != 1)
            throw std::runtime_error("The duplicate registration should have been reported exactly once!");

        easydbuspp::session_manager proxy_session_manager {easydbuspp::bus_type_t::SESSION};
        easydbuspp::proxy           proxy {proxy_session_manager, BUS_NAME, INTERFACE_NAME, OBJECT_PATH};

        if (proxy.call<std::string>("Ping") != "pong")
            throw std::runtime_error("The object that did get registered should work!");

        // An object created once the connection is up is registered by publish().
        easydbuspp::object late_object {obj_session_manager, INTERFACE_NAME, LATE_OBJECT_PATH};

        if (late_object_warnings != 1)
            throw std::runtime_error("Creating an object after the connection is up should have warned!");

        late_object.add_method("LatePing", [] {
            return std::string {"late pong"};
        });

        late_object.publish();

        easydbuspp::proxy late_proxy {proxy_session_manager, BUS_NAME, INTERFACE_NAME, LATE_OBJECT_PATH};

        if (late_proxy.call<std::string>("LatePing") != "late pong")
            throw std::runtime_error("The late object should answer after publish()!");

        // Publishing again picks up later additions.
        late_object.add_method("LatePing2", [] {
            return std::string {"later pong"};
        });

        late_object.publish();

        if (late_proxy.call<std::string>("LatePing2") != "later pong")
            throw std::runtime_error("The re-published object should answer 'LatePing2'!");

        // Registering a path and interface that are taken reports the failure to the caller.
        easydbuspp::object another_duplicate {obj_session_manager, INTERFACE_NAME, OBJECT_PATH};
        bool               exception_caught {false};

        try {
            another_duplicate.publish();
        } catch (const std::runtime_error&) {
            exception_caught = true;
        }

        if (!exception_caught)
            throw std::runtime_error("publish() of an object whose path is taken should have thrown!");

        easydbuspp::main_loop::instance().stop();
        easydbuspp::main_loop::instance().wait();

        std::cout << "Test passed.\n";
    } catch (const std::exception& e) {
        std::cerr << "Test failed: " << e.what() << '\n';
        return 1;
    }

    return 0;
}
