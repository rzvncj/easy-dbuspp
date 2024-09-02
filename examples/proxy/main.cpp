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
#include <map>
#include <proxy.h>
#include <thread>

int main()
{
    using namespace std::chrono_literals;

    try {
        const std::string BUS_NAME {"org.gtk.GDBus.Test"};

        easydbuspp::session_manager session_manager {easydbuspp::bus_type_t::SESSION};
        easydbuspp::proxy proxy {session_manager, BUS_NAME, "org.gtk.GDBus.TestInterface", "/org/gtk/GDBus/TestObject"};

        easydbuspp::org_freedesktop_dbus_proxy dbus_proxy(session_manager);
        std::string                            unique_bus_name = dbus_proxy.unique_bus_name(BUS_NAME);

        std::cout << "** Bus UID: " << dbus_proxy.uid(BUS_NAME) << "\n** Service PID: " << dbus_proxy.pid(BUS_NAME)
                  << std::endl;

        auto jazzman = proxy.call<std::string>("MostInterestingJazzMusician");

        std::cout << "====================== METHODS ==========================\n"
                  << "MostInterestingJazzMusician(): " << jazzman << "\n";

        using variant_type_t    = std::variant<std::string, bool>;
        using dictionary_type_t = std::map<std::string, variant_type_t>;

        auto dict = proxy.call<dictionary_type_t>(
            "TestDictionary",
            dictionary_type_t {{"key1", true}, {"key2", "key2_value"}, {"key3", false}, {"key4", "key4_value"}});

        for (auto&& [key, value] : dict) {
            std::cout << "dict['" << key << "']: ";

            std::visit(
                [](auto&& arg) {
                    std::cout << arg;
                },
                value);

            std::cout << "\n";
        }

        std::cout << "Calling ThrowException() - and expecting an std::exception to be thrown...\n";

        try {
            proxy.call<void>("ThrowException");
        } catch (const std::exception& e) {
            std::cout << "Exception caught, as expected: " << e.what() << "\n";
        }

        std::cout << "=================== CACHED PROPERTIES (get) =============\n";
        std::cout << "GreatestPhilosopher property: " << proxy.cached_property<std::string>("GreatestPhilosopher")
                  << "\n";

        auto free_jazz_musicians = proxy.cached_property<std::vector<std::string>>("FreeJazzMusicians");

        std::cout << "FreeJazzMusicians:\n";

        for (auto&& elem : free_jazz_musicians)
            std::cout << "'" << elem << "' ";

        std::cout << "\n==================== CACHED PROPERTIES (set) ============\n";

        free_jazz_musicians.emplace_back("John Coltrane");
        proxy.cached_property<std::vector<std::string>>("FreeJazzMusicians", free_jazz_musicians);

        auto new_value = proxy.cached_property<std::vector<std::string>>("FreeJazzMusicians");

        std::cout << "Updated FreeJazzMusicians:\n";

        for (auto&& elem : free_jazz_musicians)
            std::cout << "'" << elem << "' ";

        std::cout << "\n============ SETTING PROPERTIES VIA METHOD CALL =========\n";

        free_jazz_musicians.emplace_back("Ornette Coleman");
        proxy.property<std::vector<std::string>>("FreeJazzMusicians", free_jazz_musicians);

        new_value = proxy.cached_property<std::vector<std::string>>("FreeJazzMusicians");

        std::cout << "Updated FreeJazzMusicians:\n";

        for (auto&& elem : free_jazz_musicians)
            std::cout << "'" << elem << "' ";

        std::cout << "\n=================== WAITING FOR SIGNALS =================\n";

        session_manager.signal_subscribe("BroadcastSignal", [&session_manager](int i, const std::string& s, float f) {
            std::cout << "Got signal BroadcastSignal: [" << i << ", '" << s << "', " << f << "]\nExiting." << std::endl;
            session_manager.stop();
        });

        session_manager.signal_subscribe(
            "UnicastSignal",
            [&session_manager](const std::string& s) {
                std::cout << "Got signal UnicastSignal: ['" << s << "']" << std::endl;
            },
            unique_bus_name.c_str(), "org.gtk.GDBus.TestInterface", "/org/gtk/GDBus/TestObject");

        auto a = std::async(std::launch::async, [&session_manager] {
            session_manager.run();
        });

        std::this_thread::sleep_for(1s);
        proxy.call<void>("TriggerUnicastSignal", proxy.unique_bus_name());

        std::this_thread::sleep_for(1s);
        proxy.call<void>("TriggerBroadcastSignal");

    } catch (const std::exception& e) {
        std::cerr << "Catastrophe: " << e.what() << "\n";
    }
}
