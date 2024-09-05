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

        auto broadcast_signal = object.add_broadcast_signal<std::string, double>("BroadcastSignal");

        object.add_method("EmitBroadcastSignal", [&broadcast_signal] {
            broadcast_signal("Done calculating PI, here's the value", 3.14);
        });

        auto obj_a = std::async(std::launch::async, [&obj_session_manager] {
            obj_session_manager.run();
        });

        // Set up a proxy to access the object.
        easydbuspp::session_manager proxy_session_manager {easydbuspp::bus_type_t::SESSION};
        easydbuspp::proxy           proxy {proxy_session_manager, BUS_NAME, INTERFACE_NAME, OBJECT_PATH};

        proxy_session_manager.signal_subscribe(
            "BroadcastSignal", [&proxy_session_manager](const std::string& s, double d) {
                std::cout << "Got signal BroadcastSignal: [" << s << "', " << d << "]" << std::endl;
                proxy_session_manager.stop();
            });

        auto proxy_a = std::async(std::launch::async, [&proxy_session_manager] {
            proxy_session_manager.run();
        });

        proxy.call<void>("EmitBroadcastSignal");

        obj_session_manager.stop();

    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << "\n";
        return 1;
    }
}
