// SPDX-FileCopyrightText: © 2024 Răzvan Cojocaru <rzvncj@gmail.com>
//
// SPDX-License-Identifier: AGPL-3.0-only

#include <easydbuspp.h>
#include <iostream>

int main()
{
    try {
        const std::string               BUS_NAME {"net.test.EasyDBuspp.Test"};
        const std::string               INTERFACE_NAME {"net.test.EasyDBuspp.TestInterface"};
        const easydbuspp::object_path_t OBJECT_PATH {"/net/test/EasyDBuspp/TestObject"};

        // Set up an object.
        easydbuspp::session_manager obj_session_manager {easydbuspp::bus_type_t::SESSION, BUS_NAME};
        easydbuspp::object          object {obj_session_manager, INTERFACE_NAME, OBJECT_PATH};

        easydbuspp::dbus_context test_dc;

        object.add_method("MethodTakingAMethodContext", [&test_dc](const easydbuspp::dbus_context& dc) {
            test_dc = dc;
        });

        easydbuspp::main_loop::instance().run_async();

        // Set up a proxy to access the object.
        easydbuspp::session_manager proxy_session_manager {easydbuspp::bus_type_t::SESSION};
        easydbuspp::proxy           proxy {proxy_session_manager, BUS_NAME, INTERFACE_NAME, OBJECT_PATH};

        proxy.call<void>("MethodTakingAMethodContext");

        if (test_dc.bus_name != proxy.unique_bus_name())
            throw std::runtime_error("Bus name mismatch (expected: " + proxy.unique_bus_name()
                                     + ", got: " + test_dc.bus_name + ")!");

        if (test_dc.interface_name != INTERFACE_NAME || test_dc.object_path != OBJECT_PATH
            || test_dc.name != "MethodTakingAMethodContext")
            throw std::runtime_error("Unexpected context data received!");

        easydbuspp::main_loop::instance().stop();
        easydbuspp::main_loop::instance().wait();

    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << "\n";
        return 1;
    }
}
