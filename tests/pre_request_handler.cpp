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

        object.add_method("RunMethod", [] {
        });

        object.add_method("DontRunMethod", [] {
        });

        object.pre_request_handler(
            [&obj_session_manager](easydbuspp::object::request_type req_type, const easydbuspp::dbus_context& dc) {
                if (req_type != easydbuspp::object::request_type::METHOD)
                    throw std::runtime_error("Unexpected request type!");

                if (dc.name != "RunMethod")
                    throw std::runtime_error("Unexpected method name!");

                easydbuspp::org_freedesktop_dbus_proxy dbus_proxy(obj_session_manager);

                if (getpid() != dbus_proxy.pid(dc.bus_name))
                    throw std::runtime_error("Unexpected sender PID!");

                if (getuid() != dbus_proxy.uid(dc.bus_name))
                    throw std::runtime_error("Unexpected sender UID!");
            });

        easydbuspp::main_loop::instance().run_async();

        // Set up a proxy to access the object.
        easydbuspp::session_manager proxy_session_manager {easydbuspp::bus_type_t::SESSION};
        easydbuspp::proxy           proxy {proxy_session_manager, BUS_NAME, INTERFACE_NAME, OBJECT_PATH};

        proxy.call<void>("RunMethod");

        bool exception_caught {false};

        try {
            proxy.call<void>("DontRunMethod");
        } catch (const std::exception&) {
            exception_caught = true;
        }

        if (!exception_caught)
            throw std::runtime_error("'DontRunMethod' ran when it shouldn't have!");

        easydbuspp::main_loop::instance().stop();
        easydbuspp::main_loop::instance().wait();

    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << "\n";
        return 1;
    }
}
