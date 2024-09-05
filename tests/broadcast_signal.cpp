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

        auto broadcast_signal = object.add_broadcast_signal<std::string, double>("BroadcastSignal");

        object.add_method("EmitBroadcastSignal", [&broadcast_signal] {
            broadcast_signal("Done calculating PI, here's the value", 3.14);
        });

        auto obj_a = std::async(std::launch::async, [&obj_session_manager] {
            obj_session_manager.run();
        });

        // Set up a proxy to access the object.
        easydbuspp::session_manager proxy_session_manager {easydbuspp::bus_type_t::SESSION};
        easydbuspp::proxy proxy(proxy_session_manager, "net.test.EasyDBuspp.Test", "net.test.EasyDBuspp.TestInterface",
                                "/net/test/EasyDBuspp/TestObject");

        proxy_session_manager.signal_subscribe(
            "BroadcastSignal", [&proxy_session_manager](const std::string& s, double d) {
                std::cout << "Got signal BroadcastSignal: [" << s << "', " << d << "]\nExiting." << std::endl;
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
