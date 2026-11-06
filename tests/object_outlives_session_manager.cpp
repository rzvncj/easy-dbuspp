// SPDX-FileCopyrightText: © 2024 Răzvan Cojocaru <razvanc@mailbox.org>
//
// SPDX-License-Identifier: AGPL-3.0-only

#include "wait_for_service.h"
#include <easydbuspp.h>
#include <iostream>

int main()
{
    try {
        const std::string               BUS_NAME {"net.test.EasyDBuspp.Test"};
        const std::string               INTERFACE_NAME {"net.test.EasyDBuspp.TestInterface"};
        const easydbuspp::object_path_t OBJECT_PATH {"/net/test/EasyDBuspp/TestObject"};

        auto session_manager = std::make_unique<easydbuspp::session_manager>(easydbuspp::bus_type_t::SESSION, BUS_NAME);
        auto object          = std::make_unique<easydbuspp::object>(*session_manager, INTERFACE_NAME, OBJECT_PATH);

        object->add_method("Ping", [] {});

        const auto signal = object->add_broadcast_signal<int>("Signal");

        easydbuspp::main_loop::instance().run_async();

        wait_for_service(BUS_NAME);

        // Destroying the object after its session_manager must not touch the destroyed session_manager.
        session_manager.reset();

        object->publish();

        try {
            signal(1);
            throw std::logic_error("Emitting a signal without a session_manager should have thrown!");
        } catch (const std::runtime_error&) {
        }

        object.reset();

        easydbuspp::main_loop::instance().stop();
        easydbuspp::main_loop::instance().wait();

    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << "\n";
        return 1;
    }
}
