// SPDX-FileCopyrightText: © 2024 Răzvan Cojocaru <razvanc@mailbox.org>
//
// SPDX-License-Identifier: AGPL-3.0-only

#include "wait_for_service.h"
#include <atomic>
#include <chrono>
#include <easydbuspp.h>
#include <iostream>
#include <memory>
#include <thread>

int main()
{
    using namespace std::chrono_literals;

    try {
        const std::string               BUS_NAME {"net.test.EasyDBuspp.Test"};
        const std::string               INTERFACE_NAME {"net.test.EasyDBuspp.TestInterface"};
        const easydbuspp::object_path_t OBJECT_PATH {"/net/test/EasyDBuspp/TestObject"};

        easydbuspp::session_manager obj_session_manager {easydbuspp::bus_type_t::SESSION, BUS_NAME};
        auto object = std::make_unique<easydbuspp::object>(obj_session_manager, INTERFACE_NAME, OBJECT_PATH);

        std::atomic<bool> method_started {false};

        // The closure lives inside the object, so it's destroyed with it. Touching 'answer' after the sleep
        // is a use-after-free if the object doesn't wait for the call to finish.
        object->add_method("Slow", [&method_started, answer = 42]() {
            method_started = true;
            std::this_thread::sleep_for(500ms);
            return answer;
        });

        easydbuspp::main_loop::instance().run_async();

        wait_for_service(BUS_NAME);

        easydbuspp::session_manager proxy_session_manager {easydbuspp::bus_type_t::SESSION};
        easydbuspp::proxy           proxy {proxy_session_manager, BUS_NAME, INTERFACE_NAME, OBJECT_PATH};

        int         reply {0};
        std::thread caller {[&] {
            try {
                reply = proxy.call<int>("Slow");
            } catch (const std::exception& e) {
                std::cerr << "Call failed: " << e.what() << "\n";
            }
        }};

        while (!method_started)
            std::this_thread::sleep_for(10ms);

        const auto start = std::chrono::steady_clock::now();
        object.reset();
        const auto waited = std::chrono::steady_clock::now() - start;

        caller.join();

        if (waited < 200ms)
            throw std::runtime_error("Object destruction didn't wait for the in-flight method call!");

        if (reply != 42)
            throw std::runtime_error("In-flight method call didn't complete successfully!");

        bool exception_caught {false};

        try {
            (void)proxy.call<int>("Slow");
        } catch (const std::exception&) {
            exception_caught = true;
        }

        if (!exception_caught)
            throw std::runtime_error("Calling a destroyed object should have thrown!");

        easydbuspp::main_loop::instance().stop();
        easydbuspp::main_loop::instance().wait();

    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << "\n";
        return 1;
    }
}
