// SPDX-FileCopyrightText: © 2024 Răzvan Cojocaru <rzvncj@gmail.com>
//
// SPDX-License-Identifier: AGPL-3.0-only

#include <algorithm>
#include <easydbuspp.h>
#include <iostream>
#include <unistd.h>

int main()
{
    try {
        const std::string               BUS_NAME {"net.test.EasyDBuspp.Test"};
        const std::string               INTERFACE_NAME {"net.test.EasyDBuspp.TestInterface"};
        const easydbuspp::object_path_t OBJECT_PATH {"/net/test/EasyDBuspp/TestObject"};

        easydbuspp::session_manager obj_session_manager {easydbuspp::bus_type_t::SESSION, BUS_NAME};
        easydbuspp::object          object {obj_session_manager, INTERFACE_NAME, OBJECT_PATH};

        object.add_method("ReceiveUnixFd", [](easydbuspp::unix_fd_t fd) {
            std::cout << "Got fd: " << static_cast<uint32_t>(fd) << ", will write to it.\n";

            const std::string MESSAGE {"Hello from the object!\n"};
            write(static_cast<int>(fd), MESSAGE.c_str(), MESSAGE.length());

            return easydbuspp::unix_fd_t {STDOUT_FILENO};
        });

        easydbuspp::main_loop::instance().run();

    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << "\n";
        return 1;
    }
}
