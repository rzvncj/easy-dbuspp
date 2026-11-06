// SPDX-FileCopyrightText: © 2024 Răzvan Cojocaru <razvanc@mailbox.org>
//
// SPDX-License-Identifier: AGPL-3.0-only

#ifndef EASYDBUSPP_LIFETIME_GUARD_H_INCLUDED
#define EASYDBUSPP_LIFETIME_GUARD_H_INCLUDED

#include <condition_variable>
#include <glib.h>
#include <memory>
#include <mutex>

namespace easydbuspp {

/*
 * Shared between an object and everything that may still use it after it is gone (queued method calls, GDBus
 * dispatching on another thread). Lets the owner's destructor wait for in-flight requests and lets late requests
 * notice that the owner no longer exists.
 */
class lifetime_guard {
public:
    explicit lifetime_guard(void* owner) noexcept;

    //! Returns the owner and counts a request as in flight, or nullptr if the owner is being destroyed.
    template <typename T>
    T* enter()
    {
        return static_cast<T*>(enter_owner());
    }

    //! Ends a request counted by enter().
    void leave() noexcept;

    //! Refuses new requests and waits for the in-flight ones to finish.
    void close();

private:
    void* enter_owner();

    std::mutex              mutex_;
    std::condition_variable cv_;
    void*                   owner_;
    size_t                  in_flight_ {0};
};

using lifetime_guard_ptr = std::shared_ptr<lifetime_guard>;

/*
 * Ends a request counted by lifetime_guard::enter() when it goes out of scope. It also tracks, per thread, which
 * requests are currently being handled, so that destroying an owner from its own request can be detected.
 */
class request_scope {
public:
    explicit request_scope(lifetime_guard& guard) noexcept;
    ~request_scope() noexcept;

    request_scope(const request_scope&)            = delete;
    request_scope& operator=(const request_scope&) = delete;

    //! Is the calling thread currently handling a request guarded by 'guard'?
    static bool is_handling_request_of(const lifetime_guard& guard) noexcept;

private:
    lifetime_guard&                    guard_;
    request_scope*                     enclosing_;
    static thread_local request_scope* innermost_;
};

//! GDestroyNotify for a heap-allocated lifetime_guard_ptr that was handed to GLib as user data.
void free_lifetime_guard_ptr(gpointer data) noexcept;

} // end of namespace easydbuspp

#endif // EASYDBUSPP_LIFETIME_GUARD_H_INCLUDED
