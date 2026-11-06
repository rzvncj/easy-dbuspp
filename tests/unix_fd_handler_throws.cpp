// SPDX-FileCopyrightText: © 2024 Răzvan Cojocaru <razvanc@mailbox.org>
//
// SPDX-License-Identifier: AGPL-3.0-only

#include "wait_for_service.h"
#include <easydbuspp.h>
#include <iostream>
#include <poll.h>
#include <unistd.h>

int main()
{
    try {
        const std::string               BUS_NAME {"net.test.EasyDBuspp.Test"};
        const std::string               INTERFACE_NAME {"net.test.EasyDBuspp.TestInterface"};
        const easydbuspp::object_path_t OBJECT_PATH {"/net/test/EasyDBuspp/TestObject"};

        easydbuspp::session_manager obj_session_manager {easydbuspp::bus_type_t::SESSION, BUS_NAME};
        easydbuspp::object          object {obj_session_manager, INTERFACE_NAME, OBJECT_PATH};

        // Never closes the fd it receives, and one that it does close must not be closed again.
        object.add_method("Leaks", [](easydbuspp::unix_fd_t) {
            throw std::runtime_error("Nope!");
        });

        object.add_method("ClosesThenThrows", [](easydbuspp::unix_fd_t fd) {
            close(static_cast<int>(fd));
            throw std::runtime_error("Nope!");
        });

        easydbuspp::main_loop::instance().run_async();

        wait_for_service(BUS_NAME);

        easydbuspp::session_manager proxy_session_manager {easydbuspp::bus_type_t::SESSION};
        easydbuspp::proxy           proxy {proxy_session_manager, BUS_NAME, INTERFACE_NAME, OBJECT_PATH};

        for (const char* method : {"Leaks", "ClosesThenThrows"}) {
            int pipe_fds[2];

            if (pipe(pipe_fds) != 0)
                throw std::runtime_error("Failed to create pipe!");

            bool threw {false};

            try {
                proxy.call<void>(method, easydbuspp::unix_fd_t {pipe_fds[1]});
            } catch (const std::runtime_error&) {
                threw = true;
            }

            close(pipe_fds[1]);

            if (!threw)
                throw std::runtime_error(std::string {method} + " should have thrown!");

            // Once every copy of the write end is closed, the read end reports a hang-up.
            pollfd pfd {pipe_fds[0], POLLIN, 0};

            if (poll(&pfd, 1, 2000) != 1 || !(pfd.revents & POLLHUP))
                throw std::runtime_error(std::string {method} + ": the received fd was leaked!");

            close(pipe_fds[0]);
        }

        easydbuspp::main_loop::instance().stop();
        easydbuspp::main_loop::instance().wait();

    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << "\n";
        return 1;
    }
}
