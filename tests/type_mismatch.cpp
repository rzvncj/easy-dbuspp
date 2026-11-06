// SPDX-FileCopyrightText: © 2024 Răzvan Cojocaru <razvanc@mailbox.org>
//
// SPDX-License-Identifier: AGPL-3.0-only

#include "wait_for_service.h"
#include <atomic>
#include <chrono>
#include <easydbuspp.h>
#include <iostream>
#include <thread>

namespace {

template <typename F>
void expect_throw(const char* what, F&& f)
{
    try {
        f();
    } catch (const std::runtime_error&) {
        return;
    }

    throw std::runtime_error(std::string {what} + " should have thrown!");
}

} // end of anonymous namespace

int main()
{
    try {
        const std::string               BUS_NAME {"net.test.EasyDBuspp.Test"};
        const std::string               INTERFACE_NAME {"net.test.EasyDBuspp.TestInterface"};
        const easydbuspp::object_path_t OBJECT_PATH {"/net/test/EasyDBuspp/TestObject"};

        easydbuspp::session_manager obj_session_manager {easydbuspp::bus_type_t::SESSION, BUS_NAME};
        easydbuspp::object          object {obj_session_manager, INTERFACE_NAME, OBJECT_PATH};

        object.add_method("GetString", [] {
            return std::string {"text"};
        });

        object.add_property("IntProp", 42);

        auto broadcast_signal = object.add_broadcast_signal<std::string, int32_t>("TestSignal");

        object.add_method("EmitTestSignal", [&broadcast_signal] {
            broadcast_signal("hello", 42);
        });

        easydbuspp::main_loop::instance().run_async();
        wait_for_service(BUS_NAME);

        easydbuspp::session_manager proxy_session_manager {easydbuspp::bus_type_t::SESSION};
        easydbuspp::proxy           proxy {proxy_session_manager, BUS_NAME, INTERFACE_NAME, OBJECT_PATH};

        // Replies and cached properties of an unexpected type.
        if (proxy.call<std::string>("GetString") != "text")
            throw std::runtime_error("'GetString' returned an unexpected value!");

        expect_throw("Reading a string reply as int32_t", [&proxy] {
            (void)proxy.call<int32_t>("GetString");
        });

        expect_throw("Reading a string reply as a vector", [&proxy] {
            (void)proxy.call<std::vector<int32_t>>("GetString");
        });

        expect_throw("Reading an int property as a string", [&proxy] {
            (void)proxy.cached_property<std::string>("IntProp");
        });

        // GLib rejects invalid method names locally, without setting an error.
        expect_throw("Calling an invalid method name", [&proxy] {
            proxy.call<void>("not a valid name");
        });

        // Signals with parameters that don't match the handler's signature must not reach the handler.
        std::atomic<int> wrong_type_calls {0};
        std::atomic<int> too_few_calls {0};
        std::atomic<int> too_many_calls {0};
        std::atomic<int> good_calls {0};

        proxy_session_manager.signal_subscribe("TestSignal", [&wrong_type_calls](int32_t, int32_t) {
            ++wrong_type_calls;
        });

        proxy_session_manager.signal_subscribe("TestSignal", [&too_few_calls](const std::string&) {
            ++too_few_calls;
        });

        proxy_session_manager.signal_subscribe("TestSignal", [&too_many_calls](const std::string&, int32_t, int32_t) {
            ++too_many_calls;
        });

        proxy_session_manager.signal_subscribe("TestSignal", [&good_calls](const std::string&, int32_t) {
            ++good_calls;
        });

        proxy.call<void>("EmitTestSignal");

        const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(5);

        while (good_calls == 0 && std::chrono::steady_clock::now() < deadline)
            std::this_thread::sleep_for(std::chrono::milliseconds(20));

        std::this_thread::sleep_for(std::chrono::milliseconds(200));

        if (good_calls != 1)
            throw std::runtime_error("The matching signal handler should have been called once!");

        if (wrong_type_calls != 0 || too_few_calls != 0 || too_many_calls != 0)
            throw std::runtime_error("A handler with a mismatching signature was called!");

        easydbuspp::main_loop::instance().stop();
        easydbuspp::main_loop::instance().wait();

        std::cout << "Test passed.\n";
    } catch (const std::exception& e) {
        std::cerr << "Test failed: " << e.what() << '\n';
        return 1;
    }

    return 0;
}
