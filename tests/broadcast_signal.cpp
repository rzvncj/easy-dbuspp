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

        auto broadcast_signal = object.add_broadcast_signal<std::string, double>("BroadcastSignal");

        object.add_method("EmitBroadcastSignal", [&broadcast_signal] {
            broadcast_signal("Done calculating PI, here's the value", 3.14);
        });

        easydbuspp::main_loop::instance().run_async();

        // Set up a proxy to access the object.
        easydbuspp::session_manager proxy_session_manager {easydbuspp::bus_type_t::SESSION};
        easydbuspp::proxy           proxy {proxy_session_manager, BUS_NAME, INTERFACE_NAME, OBJECT_PATH};

        proxy_session_manager.signal_subscribe(
            "BroadcastSignal", [&proxy_session_manager](const std::string& s, double d) {
                std::cout << "Got signal BroadcastSignal: [" << s << "', " << d << "]" << std::endl;
                easydbuspp::main_loop::instance().stop();
            });

        proxy.call<void>("EmitBroadcastSignal");

        easydbuspp::main_loop::instance().wait();

    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << "\n";
        return 1;
    }
}
