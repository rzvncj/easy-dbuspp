#include <easydbuspp.h>
#include <future>
#include <iostream>
#include <unistd.h>

int main()
{
    try {
        // Set up an object.
        easydbuspp::session_manager obj_session_manager {easydbuspp::bus_type_t::SESSION, "net.test.EasyDBuspp.Test"};
        easydbuspp::object          object {obj_session_manager, "net.test.EasyDBuspp.TestInterface",
                                   "/net/test/EasyDBuspp/TestObject"};

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
        easydbuspp::proxy proxy(proxy_session_manager, "net.test.EasyDBuspp.Test", "net.test.EasyDBuspp.TestInterface",
                                "/net/test/EasyDBuspp/TestObject");

        proxy.call<void>("DummyMethod");

        obj_session_manager.stop();

    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << "\n";
        return 1;
    }
}
