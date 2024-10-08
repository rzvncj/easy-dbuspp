// SPDX-FileCopyrightText: © 2024 Răzvan Cojocaru <rzvncj@gmail.com>
//
// SPDX-License-Identifier: AGPL-3.0-only

#include <algorithm>
#include <config.h>
#include <easydbuspp.h>
#include <filesystem>
#include <iostream>

// Macro borrowed from the Mozilla source code where the encoding below is also described.
// https://searchfox.org/mozilla-central/rev/dff5e85785ddeac045390db7f97922fbdb340f09/toolkit/components/remote/RemoteUtils.cpp#16
#ifdef IS_BIG_ENDIAN
#define TO_LITTLE_ENDIAN32(x) \
    ((((x) & 0xff000000) >> 24) | (((x) & 0x00ff0000) >> 8) | (((x) & 0x0000ff00) << 8) | (((x) & 0x000000ff) << 24))
#else
#define TO_LITTLE_ENDIAN32(x) (x)
#endif

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

        std::vector<std::byte> byte_url(sizeof(int32_t) * 3 + curr_path.length() + url.length() + 2);

        /*
         * Encoding algorithm from:
         * https://searchfox.org/mozilla-central/rev/dff5e85785ddeac045390db7f97922fbdb340f09/toolkit/components/remote/RemoteUtils.cpp#43
         */
        int32_t* ip = reinterpret_cast<int32_t*>(byte_url.data());
        ip[0]       = TO_LITTLE_ENDIAN32(int32_t(2));
        ip[1]       = TO_LITTLE_ENDIAN32(int32_t(sizeof(int32_t) * 3));
        ip[2]       = TO_LITTLE_ENDIAN32(int32_t(ip[1] + curr_path.length() + 1));

        char* cp = reinterpret_cast<char*>(byte_url.data()) + ip[1];
        strncpy(cp, curr_path.c_str(), curr_path.length());
        strncpy(cp + curr_path.length() + 1, url.c_str(), url.length());

        proxy.call<void>("OpenURL", byte_url);

    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << "\n";
        return 1;
    }
}
