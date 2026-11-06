// SPDX-FileCopyrightText: © 2024 Răzvan Cojocaru <razvanc@mailbox.org>
//
// SPDX-License-Identifier: AGPL-3.0-only

#include "wait_for_service.h"
#include <algorithm>
#include <chrono>
#include <easydbuspp.h>
#include <iostream>
#include <map>
#include <mutex>
#include <thread>

int main()
{
    try {
        const std::string               BUS_NAME {"net.test.EasyDBuspp.Test"};
        const std::string               INTERFACE_NAME {"net.test.EasyDBuspp.TestInterface"};
        const easydbuspp::object_path_t OBJECT_PATH {"/net/test/EasyDBuspp/TestObject"};

        easydbuspp::session_manager obj_session_manager {easydbuspp::bus_type_t::SESSION, BUS_NAME};
        easydbuspp::object          object {obj_session_manager, INTERFACE_NAME, OBJECT_PATH};

        // The setter adjusts what it is given.
        int32_t clamped {0};
        object.add_property<int32_t>(
            "Clamped",
            [&clamped] {
                return clamped;
            },
            [&clamped](const int32_t& v) {
                clamped = std::min<int32_t>(v, 10);
                return true;
            });

        object.add_property<int32_t>("Secret", {}, [](const int32_t&) {
            return true;
        });

        int32_t plain {0};
        object.add_property("Plain", plain);

        easydbuspp::main_loop::instance().run_async();
        wait_for_service(BUS_NAME);

        easydbuspp::session_manager proxy_session_manager {easydbuspp::bus_type_t::SESSION};
        easydbuspp::proxy           proxy {proxy_session_manager, BUS_NAME, INTERFACE_NAME, OBJECT_PATH};

        std::mutex                                   mutex;
        std::vector<std::pair<std::string, int32_t>> changes;

        proxy_session_manager.signal_subscribe(
            "PropertiesChanged",
            [&](const std::string&, const std::map<std::string, std::variant<int32_t>>& changed,
                const std::vector<std::string>&) {
                const std::lock_guard lock {mutex};

                for (const auto& [name, value] : changed)
                    changes.emplace_back(name, std::get<0>(value));
            },
            {}, "org.freedesktop.DBus.Properties", OBJECT_PATH);

        auto wait_for_change = [&](const std::string& name) -> int32_t {
            const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(5);

            while (std::chrono::steady_clock::now() < deadline) {
                {
                    const std::lock_guard lock {mutex};

                    for (const auto& [n, v] : changes)
                        if (n == name)
                            return v;
                }

                std::this_thread::sleep_for(std::chrono::milliseconds(10));
            }

            throw std::runtime_error("No PropertiesChanged for '" + name + "'!");
        };

        proxy.property<int32_t>("Clamped", 50);

        if (wait_for_change("Clamped") != 10)
            throw std::runtime_error("PropertiesChanged should carry the value the setter kept, not the one sent!");

        // Signals from one sender arrive in order, so once 'Plain' shows up, a 'Secret' one would have too.
        proxy.property<int32_t>("Secret", 1);
        proxy.property<int32_t>("Plain", 3);

        if (wait_for_change("Plain") != 3)
            throw std::runtime_error("PropertiesChanged for 'Plain' carries the wrong value!");

        {
            const std::lock_guard lock {mutex};

            for (const auto& change : changes)
                if (change.first == "Secret")
                    throw std::runtime_error("A write-only property must not be announced in PropertiesChanged!");
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
