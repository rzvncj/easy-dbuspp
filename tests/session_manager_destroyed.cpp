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
        easydbuspp::object          object {obj_session_manager, INTERFACE_NAME, OBJECT_PATH};

        auto broadcast_signal = object.add_broadcast_signal<int>("Ping");

        object.add_method("EmitPing", [&broadcast_signal] {
            broadcast_signal(1);
        });

        easydbuspp::main_loop::instance().run_async();

        wait_for_service(BUS_NAME);

        easydbuspp::session_manager proxy_session_manager {easydbuspp::bus_type_t::SESSION};
        easydbuspp::proxy           proxy {proxy_session_manager, BUS_NAME, INTERFACE_NAME, OBJECT_PATH};

        std::atomic<int> signals_received {0};

        // The bus connection is shared by every session_manager in the process, so it outlives this one.
        auto subscriber = std::make_unique<easydbuspp::session_manager>(easydbuspp::bus_type_t::SESSION);

        subscriber->signal_subscribe("Ping", [&signals_received](int) {
            ++signals_received;
        });

        proxy.call<void>("EmitPing");

        for (int i = 0; i < 100 && signals_received == 0; ++i)
            std::this_thread::sleep_for(20ms);

        if (signals_received != 1)
            throw std::runtime_error("The subscribed signal was not delivered!");

        subscriber.reset();

        // Signals arriving now must not reach the destroyed session_manager.
        proxy.call<void>("EmitPing");
        std::this_thread::sleep_for(300ms);

        if (signals_received != 1)
            throw std::runtime_error("A signal was delivered after the subscriber was destroyed!");

        easydbuspp::main_loop::instance().stop();
        easydbuspp::main_loop::instance().wait();

    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << "\n";
        return 1;
    }
}
