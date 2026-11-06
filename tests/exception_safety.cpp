// SPDX-FileCopyrightText: © 2024 Răzvan Cojocaru <razvanc@mailbox.org>
//
// SPDX-License-Identifier: AGPL-3.0-only

#include <easydbuspp.h>
#include <iostream>

int main()
{
    try {
        const std::string               BUS_NAME {"net.test.EasyDBuspp.Test"};
        const std::string               INTERFACE_NAME {"net.test.EasyDBuspp.TestInterface"};
        const easydbuspp::object_path_t OBJECT_PATH {"/net/test/EasyDBuspp/TestObject"};

        easydbuspp::session_manager obj_session_manager {easydbuspp::bus_type_t::SESSION, BUS_NAME};
        easydbuspp::object          object {obj_session_manager, INTERFACE_NAME, OBJECT_PATH};

        // Not derived from std::exception.
        object.add_method("ThrowsInt", [] {
            throw 42;
        });

        object.add_property<int>(
            "ThrowsInt",
            []() -> int {
                throw 42;
            },
            [](const int&) -> bool {
                throw 42;
            });

        // Argument names that are not XML-safe must not break the introspection data.
        object.add_method("Plain",
                          [](int in) {
                              return in + 1;
                          },
                          {"a'b\"<&>"}, {"out'<&>"});

        easydbuspp::main_loop::instance().run_async();

        easydbuspp::session_manager proxy_session_manager {easydbuspp::bus_type_t::SESSION};
        easydbuspp::proxy           proxy {proxy_session_manager, BUS_NAME, INTERFACE_NAME, OBJECT_PATH};

        const auto expect_throw = [](auto&& fn, const char* what) {
            try {
                fn();
            } catch (const std::exception&) {
                return;
            }
            throw std::runtime_error(std::string {what} + " should have thrown!");
        };

        expect_throw(
            [&] {
                proxy.call<void>("ThrowsInt");
            },
            "Method throwing a non-std::exception");

        expect_throw(
            [&] {
                (void)proxy.property<int>("ThrowsInt");
            },
            "Getter throwing a non-std::exception");

        expect_throw(
            [&] {
                proxy.property("ThrowsInt", 1);
            },
            "Setter throwing a non-std::exception");

        // The service must have survived all of the above.
        if (proxy.call<int>("Plain", 4) != 5)
            throw std::runtime_error("Method with XML-special characters in its argument names did not work!");

        easydbuspp::main_loop::instance().stop();
        easydbuspp::main_loop::instance().wait();

    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << "\n";
        return 1;
    }
}
