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

        auto unicast_signal = object.add_unicast_signal<std::string>("UnicastSignal");

        object.add_method("EmitUnicastSignal", [&unicast_signal](const std::string& bus_name) {
            unicast_signal(bus_name, "Unicast signal emitted!");
        });

        auto obj_a = std::async(std::launch::async, [&obj_session_manager] {
            obj_session_manager.run();
        });

        // Set up a proxy to access the object.
        easydbuspp::session_manager proxy_session_manager {easydbuspp::bus_type_t::SESSION};
        easydbuspp::proxy           proxy {proxy_session_manager, BUS_NAME, INTERFACE_NAME, OBJECT_PATH};

        easydbuspp::org_freedesktop_dbus_proxy dbus_proxy(proxy_session_manager);
        std::string                            unique_bus_name = dbus_proxy.unique_bus_name(BUS_NAME);

        proxy_session_manager.signal_subscribe(
            "UnicastSignal",
            [&proxy_session_manager](const std::string& s) {
                std::cout << "Got signal UnicastSignal: ['" << s << "']" << std::endl;
                proxy_session_manager.stop();
            },
            unique_bus_name, INTERFACE_NAME, OBJECT_PATH);

        auto proxy_a = std::async(std::launch::async, [&proxy_session_manager] {
            proxy_session_manager.run();
        });

        proxy.call<void>("EmitUnicastSignal", proxy.unique_bus_name());

        obj_session_manager.stop();

    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << "\n";
        return 1;
    }
}
