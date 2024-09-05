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
#include <future>
#include <iostream>
#include <unistd.h>

int main()
{
    try {
        const std::string               BUS_NAME {"net.test.EasyDBuspp.Test"};
        const std::string               INTERFACE_NAME {"net.test.EasyDBuspp.TestInterface"};
        const easydbuspp::object_path_t OBJECT_PATH {"/net/test/EasyDBuspp/TestObject"};

        // Set up an object.
        easydbuspp::session_manager obj_session_manager {easydbuspp::bus_type_t::SESSION, BUS_NAME};
        easydbuspp::object          object {obj_session_manager, INTERFACE_NAME, OBJECT_PATH};

        // Just to have something to invoke.
        object.add_method("DummyMethod", [] {
        });

        object.pre_request_handler([&obj_session_manager](easydbuspp::object::request_type req_type,
                                                          const std::string&               sender_bus_name,
                                                          const std::string&               req_entity_name) {
            if (req_type != easydbuspp::object::request_type::METHOD)
                throw std::runtime_error("Unexpected request type!");

            if (req_entity_name != "DummyMethod")
                throw std::runtime_error("Unexpected method name!");

            easydbuspp::org_freedesktop_dbus_proxy dbus_proxy(obj_session_manager);

            if (getpid() != dbus_proxy.pid(sender_bus_name))
                throw std::runtime_error("Unexpected sender PID!");

            if (getuid() != dbus_proxy.uid(sender_bus_name))
                throw std::runtime_error("Unexpected sender UID!");
        });

        auto a = std::async(std::launch::async, [&obj_session_manager] {
            obj_session_manager.run();
        });

        // Set up a proxy to access the object.
        easydbuspp::session_manager proxy_session_manager {easydbuspp::bus_type_t::SESSION};
        easydbuspp::proxy           proxy {proxy_session_manager, BUS_NAME, INTERFACE_NAME, OBJECT_PATH};

        proxy.call<void>("DummyMethod");

        obj_session_manager.stop();

    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << "\n";
        return 1;
    }
}
