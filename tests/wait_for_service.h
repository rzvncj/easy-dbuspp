// SPDX-FileCopyrightText: © 2024 Răzvan Cojocaru <razvanc@mailbox.org>
//
// SPDX-License-Identifier: AGPL-3.0-only

#ifndef EASYDBUSPP_TESTS_WAIT_FOR_SERVICE_H_INCLUDED
#define EASYDBUSPP_TESTS_WAIT_FOR_SERVICE_H_INCLUDED

#include <chrono>
#include <easydbuspp.h>
#include <string>

// Bus names are acquired asynchronously, so call this after starting the main loop and before using a proxy.
inline void wait_for_service(const std::string& bus_name)
{
    easydbuspp::bus_watcher watcher {easydbuspp::bus_type_t::SESSION, bus_name};
    watcher.wait_for(std::chrono::seconds {5});
}

#endif // EASYDBUSPP_TESTS_WAIT_FOR_SERVICE_H_INCLUDED
