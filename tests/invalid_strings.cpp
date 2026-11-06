// SPDX-FileCopyrightText: © 2024 Răzvan Cojocaru <razvanc@mailbox.org>
//
// SPDX-License-Identifier: AGPL-3.0-only

#include <easydbuspp.h>
#include <iostream>

int main()
{
    using namespace easydbuspp;

    const auto expect_throw = [](auto&& fn, const char* what) {
        try {
            fn();
        } catch (const std::runtime_error&) {
            return;
        }
        throw std::runtime_error(std::string {what} + " should have thrown!");
    };

    try {
        expect_throw(
            [] {
                to_gvariant(object_path_t {"not a path"});
            },
            "An invalid object path");

        expect_throw(
            [] {
                to_gvariant(std::make_tuple(std::string {"a\xff"}));
            },
            "A string that isn't valid UTF-8");

        const g_variant_ptr path {g_variant_ref_sink(to_gvariant(object_path_t {"/net/test/Path"})), g_variant_unref};

        if (!path || !g_variant_is_of_type(path.get(), G_VARIANT_TYPE_OBJECT_PATH))
            throw std::runtime_error("A valid object path should convert!");

    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << "\n";
        return 1;
    }
}
