// SPDX-FileCopyrightText: © 2024 Răzvan Cojocaru <razvanc@mailbox.org>
//
// SPDX-License-Identifier: AGPL-3.0-only

#include <easydbuspp.h>
#include <iostream>

int main()
{
    try {
        easydbuspp::session_manager session_manager {easydbuspp::bus_type_t::SESSION};

        const auto expect_throw = [](auto&& fn, const char* what) {
            try {
                fn();
            } catch (const std::runtime_error&) {
                return;
            }
            throw std::runtime_error(std::string {what} + " should have thrown!");
        };

        expect_throw(
            [&] {
                session_manager.signal_subscribe("Not A Signal", [] {});
            },
            "An invalid signal name");

        expect_throw(
            [&] {
                session_manager.signal_subscribe("Signal", [] {}, {}, "not an interface");
            },
            "An invalid interface name");

        expect_throw(
            [&] {
                session_manager.signal_subscribe("Signal", [] {}, {}, {}, "not/a/path");
            },
            "An invalid object path");

        // Created after the connection exists, so publish() has to register it right away.
        easydbuspp::object object {session_manager, "net.test.EasyDBuspp.TestInterface", "not/a/path"};

        expect_throw(
            [&] {
                object.publish();
            },
            "Publishing an invalid object path");

        easydbuspp::object bad_interface {session_manager, "not an interface", "/net/test/EasyDBuspp/Bad"};

        expect_throw(
            [&] {
                bad_interface.publish();
            },
            "Publishing an invalid interface name");

    } catch (const std::exception& e) {
        std::cerr << "FAILED: " << e.what() << '\n';
        return 1;
    }

    return 0;
}
