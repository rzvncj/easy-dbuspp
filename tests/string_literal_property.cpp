// SPDX-FileCopyrightText: © 2024 Răzvan Cojocaru <razvanc@mailbox.org>
//
// SPDX-License-Identifier: AGPL-3.0-only

#include "wait_for_service.h"
#include <easydbuspp.h>
#include <iostream>

int main()
{
    try {
        const std::string               BUS_NAME {"net.test.EasyDBuspp.Test"};
        const std::string               INTERFACE_NAME {"net.test.EasyDBuspp.TestInterface"};
        const easydbuspp::object_path_t OBJECT_PATH {"/net/test/EasyDBuspp/TestObject"};

        easydbuspp::session_manager obj_session_manager {easydbuspp::bus_type_t::SESSION, BUS_NAME};
        easydbuspp::object          object {obj_session_manager, INTERFACE_NAME, OBJECT_PATH};

        object.add_property("Literal", "Ludwig Wittgenstein");

        // The property must own its value: the buffer goes away right after registration.
        {
            std::string temporary {"Temporary"};
            const char* c_string {temporary.c_str()};
            object.add_property("CString", c_string);
            temporary = "Overwritten, and the old buffer is gone";
        }

        easydbuspp::main_loop::instance().run_async();
        wait_for_service(BUS_NAME);

        easydbuspp::session_manager proxy_session_manager {easydbuspp::bus_type_t::SESSION};
        easydbuspp::proxy           proxy {proxy_session_manager, BUS_NAME, INTERFACE_NAME, OBJECT_PATH};

        if (proxy.property<std::string>("Literal") != "Ludwig Wittgenstein")
            throw std::runtime_error("'Literal' is not the expected value!");

        if (proxy.property<std::string>("CString") != "Temporary")
            throw std::runtime_error("'CString' is not the expected value!");

        easydbuspp::main_loop::instance().stop();
        easydbuspp::main_loop::instance().wait();

        std::cout << "Test passed.\n";
    } catch (const std::exception& e) {
        std::cerr << "Test failed: " << e.what() << '\n';
        return 1;
    }

    return 0;
}
