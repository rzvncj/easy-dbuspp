// SPDX-FileCopyrightText: © 2024 Răzvan Cojocaru <razvanc@mailbox.org>
//
// SPDX-License-Identifier: AGPL-3.0-only

#include "wait_for_service.h"
#include <csignal>
#include <easydbuspp.h>
#include <iostream>
#include <memory>
#include <sys/wait.h>
#include <unistd.h>

namespace {

// Destroys an object from one of its own method handlers, which must abort with a clear message.
[[noreturn]] void destroy_object_from_its_own_handler()
{
    const std::string               BUS_NAME {"net.test.EasyDBuspp.Test"};
    const std::string               INTERFACE_NAME {"net.test.EasyDBuspp.TestInterface"};
    const easydbuspp::object_path_t OBJECT_PATH {"/net/test/EasyDBuspp/TestObject"};

    easydbuspp::session_manager obj_session_manager {easydbuspp::bus_type_t::SESSION, BUS_NAME};
    auto object = std::make_unique<easydbuspp::object>(obj_session_manager, INTERFACE_NAME, OBJECT_PATH);

    object->add_method("SelfDestruct", [&object] {
        object.reset();
    });

    easydbuspp::main_loop::instance().run_async();

    wait_for_service(BUS_NAME);

    easydbuspp::session_manager proxy_session_manager {easydbuspp::bus_type_t::SESSION};
    easydbuspp::proxy           proxy {proxy_session_manager, BUS_NAME, INTERFACE_NAME, OBJECT_PATH};

    proxy.call<void>("SelfDestruct");
    std::exit(0);
}

} // end of anonymous namespace

int main(int argc, char** argv)
{
    if (argc > 1) {
        alarm(10); // Don't hang if the destructor deadlocks instead of aborting.
        destroy_object_from_its_own_handler();
    }

    // The scenario aborts the process, so run it in a child.
    const pid_t child = fork();

    if (child == 0) {
        execl(argv[0], argv[0], "child", static_cast<char*>(nullptr));
        _exit(127);
    }

    int status {0};

    if (child < 0 || waitpid(child, &status, 0) != child) {
        std::cerr << "Error: could not run the child process\n";
        return 1;
    }

    if (!WIFSIGNALED(status) || WTERMSIG(status) != SIGABRT) {
        std::cerr << "Error: destroying an object from its own handler should have aborted!\n";
        return 1;
    }
}
