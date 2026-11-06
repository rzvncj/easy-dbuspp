// SPDX-FileCopyrightText: © 2024 Răzvan Cojocaru <razvanc@mailbox.org>
//
// SPDX-License-Identifier: AGPL-3.0-only

#include "wait_for_service.h"
#include <easydbuspp.h>
#include <iostream>
#include <map>
#include <string>
#include <variant>
#include <vector>

namespace {

using containers_t = std::variant<std::vector<int32_t>, std::vector<std::string>, std::map<std::string, int32_t>,
                                  std::map<std::string, std::string>, std::tuple<int32_t, int32_t>,
                                  std::tuple<std::string, std::string>>;

} // end of anonymous namespace

int main()
{
    try {
        // The types used to pick a variant's alternative have to be exact, not just "some array".
        const auto expect_type = [](const GVariantType* type, const char* expected) {
            if (!g_variant_type_equal(type, G_VARIANT_TYPE(expected)))
                throw std::runtime_error(std::string {"Unexpected D-Bus type, wanted "} + expected);
        };

        expect_type(easydbuspp::to_dbus_type<std::vector<int32_t>>(), "ai");
        expect_type(easydbuspp::to_dbus_type<std::map<std::string, int32_t>>(), "a{si}");
        expect_type(easydbuspp::to_dbus_type<std::tuple<int32_t, std::string>>(), "(is)");
        expect_type(easydbuspp::to_dbus_type<easydbuspp::unix_fd_t>(), "h");
        expect_type(easydbuspp::to_dbus_type<const char*>(), "s");

        const std::string               BUS_NAME {"net.test.EasyDBuspp.Test"};
        const std::string               INTERFACE_NAME {"net.test.EasyDBuspp.TestInterface"};
        const easydbuspp::object_path_t OBJECT_PATH {"/net/test/EasyDBuspp/TestObject"};

        easydbuspp::session_manager obj_session_manager {easydbuspp::bus_type_t::SESSION, BUS_NAME};
        easydbuspp::object          object {obj_session_manager, INTERFACE_NAME, OBJECT_PATH};

        object.add_method("Echo", [](const containers_t& value) {
            return value;
        });

        easydbuspp::main_loop::instance().run_async();
        wait_for_service(BUS_NAME);

        easydbuspp::session_manager proxy_session_manager {easydbuspp::bus_type_t::SESSION};
        easydbuspp::proxy           proxy {proxy_session_manager, BUS_NAME, INTERFACE_NAME, OBJECT_PATH};

        const std::vector<containers_t> values {
            std::vector<int32_t> {1, 2, 3},
            std::vector<std::string> {"a", "b"},
            std::map<std::string, int32_t> {{"one", 1}},
            std::map<std::string, std::string> {{"one", "uno"}},
            std::tuple<int32_t, int32_t> {1, 2},
            std::tuple<std::string, std::string> {"x", "y"},
        };

        for (const auto& value : values) {
            const auto echoed = proxy.call<containers_t>("Echo", value);

            if (echoed != value)
                throw std::runtime_error("Variant alternative " + std::to_string(value.index())
                                         + " didn't survive the round trip (got alternative "
                                         + std::to_string(echoed.index()) + ")!");
        }

        easydbuspp::main_loop::instance().stop();
        easydbuspp::main_loop::instance().wait();

    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << "\n";
        return 1;
    }
}
