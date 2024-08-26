#include "session_manager.h"
#include <iostream>

std::tuple<int> f(float, const std::string&, const std::vector<std::string>&,
                  const std::tuple<std::string, std::vector<bool>, int, float>&, bool, std::any)
{
    return {42};
}

std::string g()
{
    return "Thelonious Monk";
}

int main()
{
    dbus_session_manager session_manager {G_BUS_TYPE_SESSION, "org.gtk.GDBus.TestServer"};
    auto dbus_obj_ptr = std::make_shared<dbus_object>("org.gtk.GDBus.TestInterface", "/org/gtk/GDBus/TestObject");

    dbus_obj_ptr->add_method("PlainNoParamsDoesnReturnTuple", g);
    dbus_obj_ptr->add_method("MyPlainFunction", f);
    dbus_obj_ptr->add_method("MyLambdaFunction",
                             [](uint32_t x, bool b, const std::string& s) {
                                 std::cout << x << ", " << b << ", " << s << "\n";
                                 return std::make_tuple(42, 1.2f, "woot!");
                             },
                             {"x", "b", "s"}, {"ret_i", "ret_f", "ret_str"});

    session_manager.manage(dbus_obj_ptr);
    session_manager.run();
}
