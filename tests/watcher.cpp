// SPDX-FileCopyrightText: © 2024 Răzvan Cojocaru <rzvncj@gmail.com>
//
// SPDX-License-Identifier: AGPL-3.0-only

#include <algorithm>
#include <chrono>
#include <easydbuspp.h>
#include <future>
#include <iostream>
#include <thread>

int main()
{
    using namespace std::chrono_literals;

    try {
        const std::string               BUS_NAME {"net.test.EasyDBuspp.Test"};
        const std::string               INTERFACE_NAME {"net.test.EasyDBuspp.TestInterface"};
        const easydbuspp::object_path_t OBJECT_PATH {"/net/test/EasyDBuspp/TestObject"};

        auto a = std::async(std::launch::async, [&] {
            std::this_thread::sleep_for(2s);

            // Set up an object.
            easydbuspp::session_manager obj_session_manager {easydbuspp::bus_type_t::SESSION, BUS_NAME};
            easydbuspp::object          object {obj_session_manager, INTERFACE_NAME, OBJECT_PATH};

            object.add_method("DoNothing", [] {
            });

            easydbuspp::main_loop::instance().run_async();

            std::this_thread::sleep_for(2s);

            easydbuspp::main_loop::instance().stop();
            easydbuspp::main_loop::instance().wait();
        });

        easydbuspp::bus_watcher watcher {easydbuspp::bus_type_t::SESSION, BUS_NAME};
        watcher.wait_for(4s);

        // Set up a proxy to access the object.
        easydbuspp::session_manager proxy_session_manager {easydbuspp::bus_type_t::SESSION};
        easydbuspp::proxy           proxy {proxy_session_manager, BUS_NAME, INTERFACE_NAME, OBJECT_PATH};

        proxy.call<void>("DoNothing");

        a.get();

    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << "\n";
        return 1;
    }
}
