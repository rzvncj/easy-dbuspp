// SPDX-FileCopyrightText: © 2024 Răzvan Cojocaru <rzvncj@gmail.com>
//
// SPDX-License-Identifier: AGPL-3.0-only

#include <algorithm>
#include <easydbuspp.h>
#include <iostream>

namespace {

bool did_f_run {false};

void f()
{
    did_f_run = true;
}

using variant_type_t    = std::variant<std::string, int>;
using dictionary_type_t = std::map<std::string, variant_type_t>;

// An easy-dbuspp dictionary corresponding to D-Bus' 'a{sv}' type is simply an std::map
// (or std::unordered_map) with std::string keys and std::variant values.
// std::variant<>s of what? Anything that easy-dbuspp can match to a D-Bus type.
auto square_int_values(const dictionary_type_t& m)
{
    dictionary_type_t ret;

    for (auto&& [key, value] : m) {
        if (std::holds_alternative<int>(value)) {
            const int int_value = std::get<int>(value);
            ret[key]            = int_value * int_value;
        }
    }

    return ret;
}

} // end of anonymous namespace

int main()
{
    try {
        const std::string               BUS_NAME {"net.test.EasyDBuspp.Test"};
        const std::string               INTERFACE_NAME {"net.test.EasyDBuspp.TestInterface"};
        const easydbuspp::object_path_t OBJECT_PATH {"/net/test/EasyDBuspp/TestObject"};

        // Set up an object.
        easydbuspp::session_manager obj_session_manager {easydbuspp::bus_type_t::SESSION, BUS_NAME};
        easydbuspp::object          object {obj_session_manager, INTERFACE_NAME, OBJECT_PATH};

        object.add_method("VoidPlainFunctionReturningVoid", f);

        bool did_void_ret_void_lambda_run {false};

        object.add_method("VoidLambdaReturningVoid", [&did_void_ret_void_lambda_run] {
            did_void_ret_void_lambda_run = true;
        });

        object.add_method("StringRetString", [](const std::string& name) {
            return "Hello " + name + "!";
        });

        const std::tuple<int, std::string, double> EXPECTED_VALUE {42, "Life, the Universe and Everything", 3.14};

        object.add_method("ReturnMultipleValues", [&EXPECTED_VALUE]() {
            return EXPECTED_VALUE;
        });

        object.add_method("SquareIntsInDictionaryRetNewDictionary", square_int_values);

        object.add_method("TransformVector2x", [](std::vector<uint16_t> v) {
            std::transform(v.cbegin(), v.cend(), v.cbegin(), v.begin(), std::plus<> {});
            return v;
        });

        object.add_method("ThrowException", [] {
            throw std::runtime_error("Nothing's really wrong, just testing.");
        });

        std::string byte_string;

        object.add_method("TakeAVectorOfByteAndReturnIt", [&byte_string](const std::vector<std::byte>& bytes) {
            byte_string.resize(bytes.size());
            std::transform(bytes.begin(), bytes.end(), byte_string.begin(), [](std::byte b) {
                return std::to_integer<char>(b);
            });

            return bytes;
        });

        easydbuspp::main_loop::instance().run_async();

        // Set up a proxy to access the object.
        easydbuspp::session_manager proxy_session_manager {easydbuspp::bus_type_t::SESSION};
        easydbuspp::proxy           proxy {proxy_session_manager, BUS_NAME, INTERFACE_NAME, OBJECT_PATH};

        proxy.call<void>("VoidPlainFunctionReturningVoid");

        if (!did_f_run)
            throw std::runtime_error("'VoidPlainFunctionReturningVoid' did not run!");

        proxy.call<void>("VoidLambdaReturningVoid");

        if (!did_void_ret_void_lambda_run)
            throw std::runtime_error("'VoidLambdaReturningVoid' did not run!");

        auto greeting = proxy.call<std::string>("StringRetString", "Wayne");

        if (greeting != "Hello Wayne!")
            throw std::runtime_error("'StringRetString' did not return the expected value!");

        auto multiple_values = proxy.call<decltype(EXPECTED_VALUE)>("ReturnMultipleValues");

        if (multiple_values != EXPECTED_VALUE)
            throw std::runtime_error("'ReturnMultipleValues' did not return the expected value!");

        dictionary_type_t input {{"key1", "str_value1"}, {"key2", 2}, {"key3", "str_value3"}, {"key4", 3}};

        auto squared_dict = proxy.call<dictionary_type_t>("SquareIntsInDictionaryRetNewDictionary", input);

        for (auto&& [key, value] : squared_dict) {
            if (std::holds_alternative<int>(value)) {
                const int original_int_value = std::get<int>(input[key]);

                if (std::get<int>(squared_dict[key]) != original_int_value * original_int_value)
                    throw std::runtime_error(
                        "'SquareIntsInDictionaryRetNewDictionary' did not return the expected value!");
            } else {
                if (input[key] != squared_dict[key])
                    throw std::runtime_error(
                        "'SquareIntsInDictionaryRetNewDictionary' did not return the expected value!");
            }
        }

        std::vector<uint16_t> v {1, 2, 3, 4, 5, 6};
        auto                  doubled_v = proxy.call<decltype(v)>("TransformVector2x", v);

        for (size_t i = 0; i < v.size(); ++i) {
            if (doubled_v[i] != 2 * v[i])
                throw std::runtime_error("'TransformVector2x' did not return the expected value!");
        }

        bool exception_caught {false};

        try {
            proxy.call<void>("ThrowException");
        } catch (const std::exception&) {
            exception_caught = true;
        }

        if (!exception_caught)
            throw std::runtime_error("'ThrowException' should have thrown an exception but didn't!");

        std::vector<std::byte> bytes {std::byte {'h'}, std::byte {'e'}, std::byte {'l'}, std::byte {'l'},
                                      std::byte {'o'}};

        auto bytes_out = proxy.call<std::vector<std::byte>>("TakeAVectorOfByteAndReturnIt", bytes);

        if (byte_string != "hello")
            throw std::runtime_error("Unexpected value computed by 'TakeAVectorOfByteAndReturnIt'!");

        if (bytes != bytes_out)
            throw std::runtime_error("Unexpected value returned by 'TakeAVectorOfByteAndReturnIt'!");

        easydbuspp::main_loop::instance().stop();
        easydbuspp::main_loop::instance().wait();

    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << "\n";
        return 1;
    }
}
