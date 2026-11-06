// SPDX-FileCopyrightText: © 2024 Răzvan Cojocaru <razvanc@mailbox.org>
//
// SPDX-License-Identifier: AGPL-3.0-only

#include <lifetime_guard.h>

namespace easydbuspp {

lifetime_guard::lifetime_guard(void* owner) noexcept : owner_ {owner}
{
}

void* lifetime_guard::enter_owner()
{
    const std::lock_guard lock {mutex_};

    if (!owner_)
        return nullptr;

    ++in_flight_;
    return owner_;
}

void lifetime_guard::leave() noexcept
{
    const std::lock_guard lock {mutex_};

    if (--in_flight_ == 0)
        cv_.notify_all();
}

void lifetime_guard::close()
{
    // The request would be waiting for itself, and its handler would be destroyed while running.
    if (request_scope::is_handling_request_of(*this))
        g_error("Can't destroy an easydbuspp object or session_manager from within one of its own handlers!");

    std::unique_lock lock {mutex_};

    owner_ = nullptr;
    cv_.wait(lock, [this] {
        return in_flight_ == 0;
    });
}

thread_local request_scope* request_scope::innermost_ {nullptr};

request_scope::request_scope(lifetime_guard& guard) noexcept : guard_ {guard}, enclosing_ {innermost_}
{
    innermost_ = this;
}

request_scope::~request_scope() noexcept
{
    innermost_ = enclosing_;
    guard_.leave();
}

bool request_scope::is_handling_request_of(const lifetime_guard& guard) noexcept
{
    for (const request_scope* scope = innermost_; scope; scope = scope->enclosing_)
        if (&scope->guard_ == &guard)
            return true;

    return false;
}

void free_lifetime_guard_ptr(gpointer data) noexcept
{
    delete static_cast<lifetime_guard_ptr*>(data);
}

} // end of namespace easydbuspp
