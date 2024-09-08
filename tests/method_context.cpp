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

        easydbuspp::method_context test_mc;

        object.add_method("MethodTakingAMethodContext", [&test_mc](const easydbuspp::method_context& mc) {
            test_mc = mc;
        });

        obj_session_manager.run_async();

        // Set up a proxy to access the object.
        easydbuspp::session_manager proxy_session_manager {easydbuspp::bus_type_t::SESSION};
        easydbuspp::proxy           proxy {proxy_session_manager, BUS_NAME, INTERFACE_NAME, OBJECT_PATH};

        proxy.call<void>("MethodTakingAMethodContext");

        if (test_mc.bus_name != proxy.unique_bus_name())
            throw std::runtime_error("Bus name mismatch (expected: " + proxy.unique_bus_name()
                                     + ", got: " + test_mc.bus_name + ")!");

        if (test_mc.interface_name != INTERFACE_NAME || test_mc.object_path != OBJECT_PATH
            || test_mc.method_name != "MethodTakingAMethodContext")
            throw std::runtime_error("Unexpected context data received!");

        obj_session_manager.stop();

    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << "\n";
        return 1;
    }
}
