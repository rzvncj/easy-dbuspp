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
    try {
        const std::string               BUS_NAME {"net.test.EasyDBuspp.Test"};
        const std::string               INTERFACE_NAME {"net.test.EasyDBuspp.TestInterface"};
        const easydbuspp::object_path_t OBJECT_PATH {"/net/test/EasyDBuspp/TestObject"};

        easydbuspp::session_manager obj_session_manager {easydbuspp::bus_type_t::SESSION, BUS_NAME};
        easydbuspp::object          object {obj_session_manager, INTERFACE_NAME, OBJECT_PATH};

        // Move-only and mutable callables.
        object.add_method("MoveOnly", [value = std::make_unique<int32_t>(42)] {
            return *value;
        });

        object.add_method("Counter", [count = 0]() mutable {
            return ++count;
        });

        auto broadcast_signal = object.add_broadcast_signal<int32_t>("TestSignal");

        object.add_method("EmitTestSignal", [&broadcast_signal] {
            broadcast_signal(7);
        });

        easydbuspp::main_loop::instance().run_async();
        wait_for_service(BUS_NAME);

        easydbuspp::session_manager proxy_session_manager {easydbuspp::bus_type_t::SESSION};
        easydbuspp::proxy           proxy {proxy_session_manager, BUS_NAME, INTERFACE_NAME, OBJECT_PATH};

        if (proxy.call<int32_t>("MoveOnly") != 42)
            throw std::runtime_error("'MoveOnly' returned an unexpected value!");

        if (proxy.call<int32_t>("Counter") != 1 || proxy.call<int32_t>("Counter") != 2)
            throw std::runtime_error("The state of a mutable method callable was not kept between calls!");

        std::atomic<int32_t> received {0};

        proxy_session_manager.signal_subscribe(
            "TestSignal", [received_ptr = &received, owned = std::make_unique<int32_t>(1)](int32_t value) mutable {
                received_ptr->fetch_add(value * *owned);
            });

        proxy.call<void>("EmitTestSignal");

        const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(5);

        while (received == 0 && std::chrono::steady_clock::now() < deadline)
            std::this_thread::sleep_for(std::chrono::milliseconds(20));

        if (received != 7)
            throw std::runtime_error("The move-only signal handler was not called!");

        easydbuspp::main_loop::instance().stop();
        easydbuspp::main_loop::instance().wait();

        std::cout << "Test passed.\n";
    } catch (const std::exception& e) {
        std::cerr << "Test failed: " << e.what() << '\n';
        return 1;
    }

    return 0;
}
