// SPDX-FileCopyrightText: © 2024 Răzvan Cojocaru <razvanc@mailbox.org>
//
// SPDX-License-Identifier: AGPL-3.0-only

#include <chrono>
#include <easydbuspp.h>
#include <iostream>

int main()
{
    using namespace std::chrono_literals;

    auto& detector = easydbuspp::idle_detector::instance();

    try {
        // Every round must fire the timeout and stop the loop, including the ones
        // that enable the detector after an earlier timeout has already fired.
        for (int round = 0; round < 3; ++round) {
            detector.enable(50ms);
            easydbuspp::main_loop::instance().run();
        }

        detector.disable();
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << "\n";
        return 1;
    }
}
