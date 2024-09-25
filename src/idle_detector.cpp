// SPDX-FileCopyrightText: © 2024 Răzvan Cojocaru <rzvncj@gmail.com>
//
// SPDX-License-Identifier: AGPL-3.0-only

#include <idle_detector.h>
#include <object.h>

namespace easydbuspp {

idle_detector::~idle_detector()
{
    // TODO: I'm betting that the future::get() call in disable() won't throw.
    disable();
}

idle_detector& idle_detector::instance()
{
    static idle_detector the_instance;
    return the_instance;
}

void idle_detector::disable()
{
    if (!idle_future_.valid())
        return;

    {
        std::lock_guard lock(idle_mutex_);
        stop_ = true;
    }

    idle_cv_.notify_all();

    if (idle_future_.valid())
        idle_future_.get();
}

void idle_detector::ping(const object_path_t& object_path)
{
    if (!idle_future_.valid())
        return;

    {
        std::lock_guard lock(idle_mutex_);

        if (excluded_objects_.find(object_path) != excluded_objects_.end())
            return;

        request_ping_ = true;
    }

    idle_cv_.notify_all();
}

void idle_detector::exclude(const object& obj)
{
    std::lock_guard lock(idle_mutex_);

    excluded_objects_.insert(obj.object_path());
}

} // end of namespace easydbuspp
