#include <algorithm>
#include <easydbuspp.h>
#include <future>
#include <iostream>

int main()
{
    try {
        // Set up an object.
        easydbuspp::session_manager obj_session_manager {easydbuspp::bus_type_t::SESSION, "net.test.EasyDBuspp.Test"};
        easydbuspp::object          object {obj_session_manager, "net.test.EasyDBuspp.TestInterface",
                                   "/net/test/EasyDBuspp/TestObject"};

        object.add_property("ReadOnlyLiteral", 42);

        double double_property = 3.14;
        object.add_property("ReadWriteDouble", double_property);

        std::string write_only_string_property;

        object.add_property<std::string>("WriteOnlyString", {},
                                         [&write_only_string_property](const std::string& input) {
                                             write_only_string_property = input;
                                             return true;
                                         });

        std::vector<std::string> free_jazz_musicians {"Albert Ayler", "Peter Brötzmann"};

        // Read/write property, because we're passing both a setter and a getter.
        object.add_property<std::vector<std::string>>(
            "FreeJazzMusicians",
            [&free_jazz_musicians] {
                return free_jazz_musicians;
            },
            [&free_jazz_musicians](const std::vector<std::string>& new_value) {
                free_jazz_musicians = new_value;
                return true;
            });

        auto a = std::async(std::launch::async, [&obj_session_manager] {
            obj_session_manager.run();
        });

        // Set up a proxy to access the object.
        easydbuspp::session_manager proxy_session_manager {easydbuspp::bus_type_t::SESSION};
        easydbuspp::proxy proxy(proxy_session_manager, "net.test.EasyDBuspp.Test", "net.test.EasyDBuspp.TestInterface",
                                "/net/test/EasyDBuspp/TestObject");

        if (proxy.cached_property<int>("ReadOnlyLiteral") != 42)
            throw std::runtime_error("'ReadOnlyLiteral' is not the expected value!");

        bool exception_caught {false};

        try {
            proxy.property("ReadOnlyLiteral", 43);
        } catch (const std::exception&) {
            exception_caught = true;
        }

        if (!exception_caught)
            throw std::runtime_error("Write to 'ReadOnlyLiteral' succeeded, and it shouldn't have!");

        if (proxy.cached_property<double>("ReadWriteDouble") != double_property)
            throw std::runtime_error("'ReadWriteDouble' is not the expected value!");

        const double new_double_value {1.5};
        proxy.property("ReadWriteDouble", new_double_value);

        if (proxy.cached_property<double>("ReadWriteDouble") != new_double_value)
            throw std::runtime_error("'ReadWriteDouble' (cached read) is not the expected value!");

        if (proxy.property<double>("ReadWriteDouble") != new_double_value)
            throw std::runtime_error("'ReadWriteDouble' (method read) is not the expected value!");

        proxy.property("WriteOnlyString", "test value");

        if (write_only_string_property != "test value")
            throw std::runtime_error("'WriteOnlyString' is not the expected value!");

        exception_caught = false;

        try {
            auto ret_try_to_read = proxy.property<std::string>("WriteOnlyString");
        } catch (const std::exception&) {
            exception_caught = true;
        }

        if (!exception_caught)
            throw std::runtime_error("Read from 'WriteOnlyString' succeeded, and it shouldn't have!");

        auto rw_prop_ret = proxy.cached_property<std::vector<std::string>>("FreeJazzMusicians");

        rw_prop_ret.emplace_back("John Coltrane");
        proxy.property("FreeJazzMusicians", rw_prop_ret);

        // Cached.
        auto rw_prop_ret2 = proxy.cached_property<std::vector<std::string>>("FreeJazzMusicians");

        auto it1 = std::find(rw_prop_ret2.begin(), rw_prop_ret2.end(), "John Coltrane");
        auto it2 = std::find(rw_prop_ret2.begin(), rw_prop_ret2.end(), "Albert Ayler");

        if (it1 == rw_prop_ret2.end() || it2 == rw_prop_ret2.end())
            throw std::runtime_error("'FreeJazzMusicians' (cached read) is not the expected value!");

        // Remote method call.
        auto rw_prop_ret3 = proxy.property<std::vector<std::string>>("FreeJazzMusicians");

        it1 = std::find(rw_prop_ret3.begin(), rw_prop_ret3.end(), "John Coltrane");
        it2 = std::find(rw_prop_ret3.begin(), rw_prop_ret3.end(), "Albert Ayler");

        if (it1 == rw_prop_ret3.end() || it2 == rw_prop_ret3.end())
            throw std::runtime_error("'FreeJazzMusicians' (method read) is not the expected value!");

        obj_session_manager.stop();

    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << "\n";
        return 1;
    }
}
