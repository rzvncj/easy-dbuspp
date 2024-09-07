// easy-dbuspp
// Copyright (C) 2024-  Răzvan Cojocaru <rzvncj@gmail.com>
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU Affero General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU Affero General Public License for more details.
//
// You should have received a copy of the GNU Affero General Public License
// along with this program.  If not, see <http://www.gnu.org/licenses/>.

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
                          [](const easydbuspp::method_context&, int, float, const std::string&) {
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
