// SPDX-FileCopyrightText: © 2024 Răzvan Cojocaru <rzvncj@gmail.com>
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

        // Set up an object.
        easydbuspp::session_manager obj_session_manager {easydbuspp::bus_type_t::SESSION, BUS_NAME};
        easydbuspp::object          object {obj_session_manager, INTERFACE_NAME, OBJECT_PATH};

        object.add_method("InputParameterNames",
                          [](int, float, const std::string&) {
                          },
                          {"i_param", "f_param", "s_param"});

        bool exception_caught {false};

        try {
            object.add_method("InputParameterNamesTooManyNames",
                              [](int, float, const std::string&) {
                              },
                              {"i_param", "f_param", "s_param", "doesnt_match_anything"});
        } catch (const std::exception&) {
            exception_caught = true;
        }

        if (!exception_caught)
            throw std::runtime_error("Passed too many input parameter names in and no exception!");

        exception_caught = false;

        try {
            object.add_method("InputParameterNamesTooFewNames",
                              [](int, float, const std::string&) {
                              },
                              {"i_param", "f_param"});
        } catch (const std::exception&) {
            exception_caught = true;
        }

        if (!exception_caught)
            throw std::runtime_error("Passed too few input parameter names in and no exception!");

        object.add_method("InputParameterNamesWithMethodContext",
                          [](const easydbuspp::dbus_context&, int, float, const std::string&) {
                          },
                          {"i_param", "f_param", "s_param"});

        object.add_method("InputParameterNamesAndOutputParameterNames",
                          [](int, float, const std::string&) {
                              return std::tuple<double, bool, std::byte> {};
                          },
                          {"i_param", "f_param", "s_param"}, {"double_out_param", "bool_out_param", "byte_out_param"});

        exception_caught = false;

        try {
            object.add_method("InputParameterNamesAndTooManyOutputParameterNames",
                              [](int, float, const std::string&) {
                                  return std::tuple<double, bool, std::byte> {};
                              },
                              {"i_param", "f_param", "s_param"},
                              {"double_out_param", "bool_out_param", "byte_out_param", "inexistent_out_param"});
        } catch (const std::exception&) {
            exception_caught = true;
        }

        if (!exception_caught)
            throw std::runtime_error("Passed too many output parameter names in and no exception!");

        exception_caught = false;

        try {
            object.add_method("InputParameterNamesAndTooFewOutputParameterNames",
                              [](int, float, const std::string&) {
                                  return std::tuple<double, bool, std::byte> {};
                              },
                              {"i_param", "f_param", "s_param"}, {"double_out_param", "bool_out_param"});
        } catch (const std::exception&) {
            exception_caught = true;
        }

        if (!exception_caught)
            throw std::runtime_error("Passed too few output parameter names in and no exception!");

    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << "\n";
        return 1;
    }
}
