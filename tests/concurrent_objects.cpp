// SPDX-FileCopyrightText: © 2024 Răzvan Cojocaru <razvanc@mailbox.org>
//
// SPDX-License-Identifier: AGPL-3.0-only

#include "wait_for_service.h"
#include <atomic>
#include <easydbuspp.h>
#include <iostream>
#include <thread>
#include <vector>

// Meant to be run under ThreadSanitizer (-Db_sanitize=thread) as well: objects are created and destroyed, and
// signals emitted, from other threads while the main loop is acquiring the bus connection.
int main()
{
    try {
        const std::string               BUS_NAME {"net.test.EasyDBuspp.Test"};
        const std::string               INTERFACE_NAME {"net.test.EasyDBuspp.TestInterface"};
        const easydbuspp::object_path_t OBJECT_PATH {"/net/test/EasyDBuspp/TestObject"};

        easydbuspp::session_manager obj_session_manager {easydbuspp::bus_type_t::SESSION, BUS_NAME};
        easydbuspp::object          object {obj_session_manager, INTERFACE_NAME, OBJECT_PATH};

        auto broadcast_signal = object.add_broadcast_signal<int32_t>("TestSignal");

        std::atomic<bool> failed {false};
        std::atomic<int>  signals_sent {0};

        std::vector<std::thread> threads;

        // Attach and detach objects while the bus connection is being acquired and afterwards.
        threads.emplace_back([&] {
            try {
                for (int i = 0; i < 30; ++i) {
                    easydbuspp::object temporary {
                        obj_session_manager, INTERFACE_NAME,
                        easydbuspp::object_path_t {"/net/test/EasyDBuspp/Temp" + std::to_string(i)}};

                    temporary.add_method("Nothing", [] {
                    });
                }
            } catch (const std::exception& e) {
                std::cerr << "Object thread failed: " << e.what() << '\n';
                failed = true;
            }
        });

        // Emit signals from other threads; until the connection is up this is reported with an exception.
        for (int t = 0; t < 2; ++t) {
            threads.emplace_back([&] {
                for (int i = 0; i < 2000; ++i) {
                    try {
                        broadcast_signal(i);
                        ++signals_sent;
                    } catch (const std::runtime_error&) {
                    }
                }
            });
        }

        easydbuspp::main_loop::instance().run_async();
        wait_for_service(BUS_NAME);

        for (auto&& thread : threads)
            thread.join();

        if (failed)
            throw std::runtime_error("One of the threads failed!");

        // Once the connection is up, emitting works.
        broadcast_signal(0);

        easydbuspp::main_loop::instance().stop();
        easydbuspp::main_loop::instance().wait();

        std::cout << "Test passed.\n";
    } catch (const std::exception& e) {
        std::cerr << "Test failed: " << e.what() << '\n';
        return 1;
    }

    return 0;
}
