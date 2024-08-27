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

#include <iostream>
#include <map>
#include <object.h>
#include <session_manager.h>

// A pair an a tuple are treated exactly the same. For the purposes of easy-dbuspp, a pair
// is simply a tuple with two elements.
std::tuple<int> test_pair_and_tuple_return(float f, bool b, const std::pair<int, std::string>& p)
{
    std::cout << "f: " << f << ", b: " << b << "\npair: [" << std::get<0>(p) << ", " << std::get<1>(p) << "]"
              << std::endl;

    return {42};
}

using variant_type_t    = std::variant<std::string, bool>;
using dictionary_type_t = std::map<std::string, variant_type_t>;

// An easy-dbuspp dictionary corresponding to D-Bus' 'a{sv}' type is simply an std::map
// (or std::unordered_map) with std::string keys and std::variant values.
// std::variant<>s of what? Anything that easy-dbuspp can match to a D-Bus type.
auto test_dictionary(const dictionary_type_t& m)
{
    for (auto&& [key, value] : m) {
        std::cout << "m['" << key << "']: ";
        std::visit(
            [](auto&& arg) {
                std::cout << arg << std::endl;
            },
            value);
    }

    return m;
}

std::string most_interesting_jazz_musician()
{
    return "Thelonious Monk";
}

int main()
{
    try {
        easydbuspp::session_manager session_manager {G_BUS_TYPE_SESSION, "org.gtk.GDBus.Test"};
        easydbuspp::object object {session_manager, "org.gtk.GDBus.TestInterface", "/org/gtk/GDBus/TestObject"};

        object.add_method("MostInterestingJazzMusician", most_interesting_jazz_musician);
        object.add_method("TestPairAndTupleReturn", test_pair_and_tuple_return);

        // Method with parameter names specified. The first vector is input parameter names.
        // The second is output parameter names, and they match the return tuple elements, respectively.
        object.add_method("LambdaFunction",
                          [](uint32_t x, bool& b, const std::string& s) {
                              std::cout << x << ", " << b << ", " << s << "\n";
                              return std::make_tuple(42, 1.2f, "woot!");
                          },
                          {"x", "b", "s"}, {"out_i", "out_f", "out_str"});

        object.add_method("TestDictionary", test_dictionary);

        auto broadcast_signal = object.add_broadcast_signal<int, std::string, float>("BroadcastSignal");

        object.add_method("TriggerBroadcastSignal", [&broadcast_signal] {
            broadcast_signal(42, "Broadcast signal emitted!", 3.14f);
        });

        auto unicast_signal = object.add_unicast_signal<std::string>("UnicastSignal", {"destination_dbus_unique_name"});

        object.add_method("TriggerUnicastSignal", [&unicast_signal](const std::string& bus_name) {
            unicast_signal(bus_name.c_str(), "Unicast signal emitted!");
        });

        object.add_method("ThrowException", [] {
            throw std::runtime_error("Something's off!");
        });

        // Read/write property, because add_property() sees an lvalue.
        std::string strprop {"This value can be modified"};
        object.add_property("ReadWriteStringProp", strprop);

        // Read-only property because 42 is an int literal.
        object.add_property("TheAnswerToLifeTheUniverseAndEverything", 42);

        // Read-only property, because add_property() sees a const value.
        const double d {2.34f};
        object.add_property("ReadOnlyDoubleProp", d);

        object.add_property("GreatestPhilosopher", "Ludwig Wittgenstein");

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

        session_manager.run();

    } catch (const std::exception& e) {
        std::cerr << "Catastrophe: " << e.what() << "\n";
    }
}
