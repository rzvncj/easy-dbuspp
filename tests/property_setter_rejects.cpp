// SPDX-FileCopyrightText: © 2024 Răzvan Cojocaru <razvanc@mailbox.org>
//
// SPDX-License-Identifier: AGPL-3.0-only

#include <chrono>
#include <easydbuspp.h>
#include <iostream>
#include <thread>

int main()
{
    using namespace std::chrono_literals;

    try {
        const std::string               BUS_NAME {"net.test.EasyDBuspp.Test"};
        const std::string               INTERFACE_NAME {"net.test.EasyDBuspp.TestInterface"};
        const easydbuspp::object_path_t OBJECT_PATH {"/net/test/EasyDBuspp/TestObject"};

        easydbuspp::session_manager obj_session_manager {easydbuspp::bus_type_t::SESSION, BUS_NAME};
        easydbuspp::object          object {obj_session_manager, INTERFACE_NAME, OBJECT_PATH};

        int value {1};

        // The setter rejects negative values by returning false.
        object.add_property<int>(
            "PositiveInt",
            [&value] {
                return value;
            },
            [&value](const int& new_value) {
                if (new_value < 0)
                    return false;

                value = new_value;
                return true;
            });

        easydbuspp::main_loop::instance().run_async();

        easydbuspp::session_manager proxy_session_manager {easydbuspp::bus_type_t::SESSION};
        easydbuspp::proxy           proxy {proxy_session_manager, BUS_NAME, INTERFACE_NAME, OBJECT_PATH};

        bool exception_caught {false};

        try {
            proxy.property("PositiveInt", -5);
        } catch (const std::exception&) {
            exception_caught = true;
        }

        if (!exception_caught)
            throw std::runtime_error("Setting a rejected value should have thrown!");

        // Give a (wrongly) emitted PropertiesChanged signal time to reach the proxy cache.
        std::this_thread::sleep_for(300ms);

        if (proxy.property<int>("PositiveInt") != 1)
            throw std::runtime_error("Rejected value was stored!");

        if (proxy.cached_property<int>("PositiveInt") != 1)
            throw std::runtime_error("Rejected value leaked into the proxy cache via PropertiesChanged!");

        proxy.property("PositiveInt", 7);

        if (proxy.property<int>("PositiveInt") != 7)
            throw std::runtime_error("Accepted value was not stored!");

        easydbuspp::main_loop::instance().stop();
        easydbuspp::main_loop::instance().wait();

    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << "\n";
        return 1;
    }
}
