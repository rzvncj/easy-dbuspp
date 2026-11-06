// SPDX-FileCopyrightText: © 2024 Răzvan Cojocaru <razvanc@mailbox.org>
//
// SPDX-License-Identifier: AGPL-3.0-only

#include <chrono>
#include <cstddef>
#include <cstdint>
#include <easydbuspp.h>
#include <filesystem>
#include <iostream>
#include <string>
#include <vector>

namespace {

// Appends a 32-bit integer in little-endian order, whatever the endianness of this machine.
void append_le32(std::vector<std::byte>& out, uint32_t value)
{
    for (int shift = 0; shift < 32; shift += 8)
        out.push_back(static_cast<std::byte>((value >> shift) & 0xff));
}

// Appends the string and its terminating NUL.
void append_c_string(std::vector<std::byte>& out, const std::string& text)
{
    for (const char c : text)
        out.push_back(static_cast<std::byte>(c));

    out.push_back(std::byte {0});
}

} // end of anonymous namespace

int main(int argc, char** argv)
{
    using namespace std::chrono_literals;

    try {
        if (argc < 2) {
            std::cerr << "No URL provided!\n";
            return 1;
        }

        easydbuspp::bus_watcher watcher {easydbuspp::bus_type_t::SESSION, "org.mozilla.firefox.SearchProvider"};

        easydbuspp::main_loop::instance().run_async();

        watcher.wait_for(10s);

        easydbuspp::session_manager proxy_session_manager {easydbuspp::bus_type_t::SESSION};
        easydbuspp::proxy proxy {proxy_session_manager, "org.mozilla.firefox.SearchProvider", "org.mozilla.firefox",
                                 "/org/mozilla/firefox/Remote"};

        const std::string curr_path {std::filesystem::current_path()};
        const std::string url {argv[1]};

        /*
         * Encoding algorithm from:
         * https://searchfox.org/mozilla-central/rev/dff5e85785ddeac045390db7f97922fbdb340f09/toolkit/components/remote/RemoteUtils.cpp#43
         * A header of three integers (the number of strings, and the offsets of the two strings), followed by
         * the working directory and the URL.
         */
        constexpr uint32_t HEADER_SIZE {sizeof(uint32_t) * 3};

        std::vector<std::byte> byte_url;
        append_le32(byte_url, 2);
        append_le32(byte_url, HEADER_SIZE);
        append_le32(byte_url, static_cast<uint32_t>(HEADER_SIZE + curr_path.length() + 1));
        append_c_string(byte_url, curr_path);
        append_c_string(byte_url, url);

        proxy.call<void>("OpenURL", byte_url);

    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << "\n";
        return 1;
    }
}
