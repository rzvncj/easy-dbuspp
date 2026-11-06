// SPDX-FileCopyrightText: © 2024 Răzvan Cojocaru <razvanc@mailbox.org>
//
// SPDX-License-Identifier: AGPL-3.0-only

#include <chrono>
#include <cstdlib>
#include <easydbuspp.h>
#include <future>
#include <iostream>
#include <memory>

int main()
{
    using namespace std::chrono_literals;

    try {
        const std::string BUS_NAME {"net.test.EasyDBuspp.Test"};

        easydbuspp::main_loop::instance().run_async();

        easydbuspp::bus_watcher watcher {easydbuspp::bus_type_t::SESSION, BUS_NAME};

        std::promise<void> disappeared;
        auto               disappeared_future = disappeared.get_future();

        // Replacing the handler from inside the handler must not deadlock.
        watcher.bus_name_disappeared_handler([&watcher, &disappeared] {
            watcher.bus_name_disappeared_handler([] {
            });
            disappeared.set_value();
        });

        auto owner = std::make_unique<easydbuspp::session_manager>(easydbuspp::bus_type_t::SESSION, BUS_NAME);
        watcher.wait_for(5s);

        owner.reset();

        if (disappeared_future.wait_for(5s) != std::future_status::ready) {
            // The main loop thread is stuck, so stopping it would hang too.
            std::cerr << "Test failed: the disappeared handler deadlocked!\n";
            std::_Exit(1);
        }

        easydbuspp::main_loop::instance().stop();
        easydbuspp::main_loop::instance().wait();

        std::cout << "Test passed.\n";
    } catch (const std::exception& e) {
        std::cerr << "Test failed: " << e.what() << '\n';
        return 1;
    }

    return 0;
}
