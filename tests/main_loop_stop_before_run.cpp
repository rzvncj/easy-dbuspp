// SPDX-FileCopyrightText: © 2024 Răzvan Cojocaru <razvanc@mailbox.org>
//
// SPDX-License-Identifier: AGPL-3.0-only

#include <chrono>
#include <easydbuspp.h>
#include <future>
#include <iostream>

int main()
{
    using namespace std::chrono_literals;

    try {
        auto& loop = easydbuspp::main_loop::instance();

        // A stop() that races with (or precedes) the start of the loop must not be lost.
        for (int i = 0; i < 100; ++i) {
            loop.run_async();
            loop.stop();

            auto waiter = std::async(std::launch::async, [&loop] {
                loop.wait();
            });

            if (waiter.wait_for(5s) != std::future_status::ready)
                throw std::runtime_error("main_loop::stop() was lost: wait() never returned!");
        }

        // The loop must still be usable afterwards.
        loop.run_async();
        loop.stop();
        loop.wait();

        std::cout << "Test passed.\n";
    } catch (const std::exception& e) {
        std::cerr << "Test failed: " << e.what() << '\n';
        return 1;
    }

    return 0;
}
