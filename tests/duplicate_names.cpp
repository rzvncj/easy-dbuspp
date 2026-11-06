// SPDX-FileCopyrightText: © 2024 Răzvan Cojocaru <razvanc@mailbox.org>
//
// SPDX-License-Identifier: AGPL-3.0-only

#include <easydbuspp.h>
#include <iostream>

int main()
{
    try {
        easydbuspp::session_manager session_manager {easydbuspp::bus_type_t::SESSION};
        easydbuspp::object          object {session_manager, "net.test.EasyDBuspp.TestInterface",
                                   "/net/test/EasyDBuspp/TestObject"};

        const auto expect_throw = [](auto&& fn, const char* what) {
            try {
                fn();
            } catch (const std::runtime_error&) {
                return;
            }
            throw std::runtime_error(std::string {what} + " should have thrown!");
        };

        int value {0};

        object.add_method("Method", [] {});
        object.add_property("Value", value);

        const auto signal = object.add_broadcast_signal<int>("Signal");

        expect_throw(
            [&] {
                object.add_method("Method", [] {});
            },
            "A duplicate method");

        expect_throw(
            [&] {
                object.add_property("Value", value);
            },
            "A duplicate property");

        expect_throw(
            [&] {
                object.add_property<int>(
                    "Value",
                    [] {
                        return 1;
                    },
                    {});
            },
            "A duplicate getter/setter property");

        expect_throw(
            [&] {
                const auto duplicate = object.add_unicast_signal<int>("Signal");
            },
            "A duplicate signal");

    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << "\n";
        return 1;
    }
}
