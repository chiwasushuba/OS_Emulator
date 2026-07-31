#pragma once
#include <windows.h>

namespace std {
class mutex {
public:
    mutex() noexcept {
        InitializeCriticalSection(&cs_);
    }

    ~mutex() noexcept {
        DeleteCriticalSection(&cs_);
    }

    mutex(const mutex&) = delete;
    mutex& operator=(const mutex&) = delete;

    void lock() noexcept {
        EnterCriticalSection(&cs_);
    }

    bool try_lock() noexcept {
        return TryEnterCriticalSection(&cs_) != 0;
    }

    void unlock() noexcept {
        LeaveCriticalSection(&cs_);
    }

private:
    CRITICAL_SECTION cs_{};
};

template <typename Mutex>
class lock_guard {
public:
    explicit lock_guard(Mutex& mutex) : mutex_(mutex) {
        mutex_.lock();
    }

    ~lock_guard() {
        mutex_.unlock();
    }

    lock_guard(const lock_guard&) = delete;
    lock_guard& operator=(const lock_guard&) = delete;

private:
    Mutex& mutex_;
};
} // namespace std
