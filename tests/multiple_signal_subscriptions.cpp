// SPDX-FileCopyrightText: © 2024 Răzvan Cojocaru <razvanc@mailbox.org>
//
// SPDX-License-Identifier: AGPL-3.0-only

#include "wait_for_service.h"
#include <atomic>
#include <chrono>
#include <easydbuspp.h>
#include <iostream>
#include <thread>

int main()
{
    try {
        const std::string               BUS_NAME {"net.test.EasyDBuspp.Test"};
        const std::string               INTERFACE_NAME {"net.test.EasyDBuspp.TestInterface"};
        const easydbuspp::object_path_t OBJECT_PATH {"/net/test/EasyDBuspp/TestObject"};

        // Set up an object with a broadcast signal and a method to trigger it.
        easydbuspp::session_manager obj_session_manager {easydbuspp::bus_type_t::SESSION, BUS_NAME};
        easydbuspp::object          object {obj_session_manager, INTERFACE_NAME, OBJECT_PATH};

        auto broadcast_signal = object.add_broadcast_signal<std::string, int32_t>("TestSignal");

        object.add_method("EmitTestSignal", [&broadcast_signal] {
            broadcast_signal("hello", 42);
        });

        easydbuspp::main_loop::instance().run_async();
        wait_for_service(BUS_NAME);

        // Set up a proxy to access the object.
        easydbuspp::session_manager proxy_session_manager {easydbuspp::bus_type_t::SESSION};
        easydbuspp::proxy           proxy {proxy_session_manager, BUS_NAME, INTERFACE_NAME, OBJECT_PATH};

        // Subscribe to the same signal twice with two independent handlers: each must be called once per signal.
        std::atomic<int> handler_1_calls {0};
        std::atomic<int> handler_2_calls {0};

        proxy_session_manager.signal_subscribe("TestSignal", [&handler_1_calls](const std::string& s, int32_t i) {
            if (s != "hello" || i != 42)
                throw std::runtime_error("Handler 1: unexpected signal parameters!");

            ++handler_1_calls;
        });

        proxy_session_manager.signal_subscribe("TestSignal", [&handler_2_calls](const std::string& s, int32_t i) {
            if (s != "hello" || i != 42)
                throw std::runtime_error("Handler 2: unexpected signal parameters!");

            ++handler_2_calls;
        });

        proxy.call<void>("EmitTestSignal");

        // Wait for both handlers to fire (with a timeout to avoid hanging), then give stray calls time to show up.
        const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(5);

        while ((handler_1_calls == 0 || handler_2_calls == 0) && std::chrono::steady_clock::now() < deadline)
            std::this_thread::sleep_for(std::chrono::milliseconds(20));

        std::this_thread::sleep_for(std::chrono::milliseconds(200));

        if (handler_1_calls != 1 || handler_2_calls != 1)
            throw std::runtime_error("Each handler should have been called once, but handler 1 was called "
                                     + std::to_string(handler_1_calls) + " times and handler 2 "
                                     + std::to_string(handler_2_calls) + " times!");

        easydbuspp::main_loop::instance().stop();
        easydbuspp::main_loop::instance().wait();

    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << "\n";
        return 1;
    }
}
