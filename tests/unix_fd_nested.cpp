// SPDX-FileCopyrightText: © 2024 Răzvan Cojocaru <razvanc@mailbox.org>
//
// SPDX-License-Identifier: AGPL-3.0-only

#include "wait_for_service.h"
#include <easydbuspp.h>
#include <filesystem>
#include <iostream>
#include <map>
#include <unistd.h>

namespace {

using easydbuspp::unix_fd_t;

size_t open_fd_count()
{
    return static_cast<size_t>(
        std::distance(std::filesystem::directory_iterator {"/proc/self/fd"}, std::filesystem::directory_iterator {}));
}

std::string read_all(int fd)
{
    char    buf[64] {};
    ssize_t n = read(fd, buf, sizeof(buf) - 1);

    return n > 0 ? std::string {buf, static_cast<size_t>(n)} : std::string {};
}

void check(bool condition, const char* what)
{
    if (!condition)
        throw std::runtime_error(what);
}

} // end of anonymous namespace

int main()
{
    try {
        // Fds nested in containers go through the fd list, not just top-level ones.
        {
            int pipes[2][2];
            check(pipe(pipes[0]) == 0 && pipe(pipes[1]) == 0, "pipe() failed!");

            std::tuple<std::vector<unix_fd_t>, std::map<std::string, unix_fd_t>> args {
                {unix_fd_t {pipes[0][1]}}, {{"key", unix_fd_t {pipes[1][1]}}}};

            auto fd_list = easydbuspp::extract_g_unix_fd_list(args);
            check(fd_list && g_unix_fd_list_get_length(fd_list.get()) == 2, "Nested fds were not put in the fd list!");
            check(static_cast<int>(std::get<0>(args)[0]) == 0 && static_cast<int>(std::get<1>(args)["key"]) == 1,
                  "Nested fds were not replaced with their indexes!");

            easydbuspp::set_up_from_g_unix_fd_list(fd_list.get(), args);

            const int first  = static_cast<int>(std::get<0>(args)[0]);
            const int second = static_cast<int>(std::get<1>(args)["key"]);

            check(write(first, "a", 1) == 1 && write(second, "b", 1) == 1, "Could not write through the new fds!");
            check(read_all(pipes[0][0]) == "a" && read_all(pipes[1][0]) == "b", "Nested fds reached the wrong place!");

            for (const int fd : {pipes[0][0], pipes[0][1], pipes[1][0], pipes[1][1], first, second})
                close(fd);
        }

        // A failure midway must not leak the fds that were already extracted.
        {
            int p[2];
            check(pipe(p) == 0, "pipe() failed!");

            std::tuple<unix_fd_t> one {unix_fd_t {p[1]}};
            auto                  fd_list = easydbuspp::extract_g_unix_fd_list(one);

            std::tuple<unix_fd_t, unix_fd_t> bad {unix_fd_t {0}, unix_fd_t {5}};
            const size_t                     before = open_fd_count();
            bool                             threw {false};

            try {
                easydbuspp::set_up_from_g_unix_fd_list(fd_list.get(), bad);
            } catch (const std::runtime_error&) {
                threw = true;
            }

            check(threw, "An out of range fd index should have thrown!");
            check(open_fd_count() == before, "An fd leaked when extraction failed!");

            close(p[0]);
            close(p[1]);
        }

        // End to end: a vector of fds as a method parameter.
        const std::string               BUS_NAME {"net.test.EasyDBuspp.Test"};
        const std::string               INTERFACE_NAME {"net.test.EasyDBuspp.TestInterface"};
        const easydbuspp::object_path_t OBJECT_PATH {"/net/test/EasyDBuspp/TestObject"};

        easydbuspp::session_manager obj_session_manager {easydbuspp::bus_type_t::SESSION, BUS_NAME};
        easydbuspp::object          object {obj_session_manager, INTERFACE_NAME, OBJECT_PATH};

        object.add_method("WriteToAll", [](const std::vector<unix_fd_t>& fds) {
            for (const auto fd : fds) {
                (void)!write(static_cast<int>(fd), "x", 1);
                close(static_cast<int>(fd));
            }

            return static_cast<int32_t>(fds.size());
        });

        easydbuspp::main_loop::instance().run_async();
        wait_for_service(BUS_NAME);

        easydbuspp::session_manager proxy_session_manager {easydbuspp::bus_type_t::SESSION};
        easydbuspp::proxy           proxy {proxy_session_manager, BUS_NAME, INTERFACE_NAME, OBJECT_PATH};

        int pipes[2][2];
        check(pipe(pipes[0]) == 0 && pipe(pipes[1]) == 0, "pipe() failed!");

        const std::vector<unix_fd_t> fds {unix_fd_t {pipes[0][1]}, unix_fd_t {pipes[1][1]}};

        check(proxy.call<int32_t>("WriteToAll", fds) == 2, "The object did not receive both fds!");
        check(read_all(pipes[0][0]) == "x" && read_all(pipes[1][0]) == "x",
              "The object did not write through the fds!");

        for (const int fd : {pipes[0][0], pipes[0][1], pipes[1][0], pipes[1][1]})
            close(fd);

        easydbuspp::main_loop::instance().stop();
        easydbuspp::main_loop::instance().wait();

        std::cout << "Test passed.\n";
    } catch (const std::exception& e) {
        std::cerr << "Test failed: " << e.what() << '\n';
        return 1;
    }

    return 0;
}
