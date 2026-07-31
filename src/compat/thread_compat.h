#pragma once

#include <windows.h>
#include <functional>
#include <chrono>

namespace std {

class thread {
public:
    thread() noexcept : handle_(nullptr), id_(0) {}
    
    template <typename Function, typename... Args>
    explicit thread(Function&& f, Args&&... args) {
        auto* p = new std::function<void()>(std::bind(std::forward<Function>(f), std::forward<Args>(args)...));
        handle_ = CreateThread(nullptr, 0, &thread::ThreadFunc, p, 0, &id_);
    }
    
    ~thread() {
        if (joinable()) {
            std::terminate();
        }
    }
    
    thread(thread&& other) noexcept : handle_(other.handle_), id_(other.id_) {
        other.handle_ = nullptr;
        other.id_ = 0;
    }
    
    thread& operator=(thread&& other) noexcept {
        if (joinable()) std::terminate();
        handle_ = other.handle_;
        id_ = other.id_;
        other.handle_ = nullptr;
        other.id_ = 0;
        return *this;
    }
    
    thread(const thread&) = delete;
    thread& operator=(const thread&) = delete;
    
    bool joinable() const noexcept { return handle_ != nullptr; }
    
    void join() {
        if (handle_) {
            WaitForSingleObject(handle_, INFINITE);
            CloseHandle(handle_);
            handle_ = nullptr;
        }
    }
    
    void detach() {
        if (handle_) {
            CloseHandle(handle_);
            handle_ = nullptr;
        }
    }

private:
    static DWORD WINAPI ThreadFunc(LPVOID lpParam) {
        auto* func = static_cast<std::function<void()>*>(lpParam);
        (*func)();
        delete func;
        return 0;
    }
    
    HANDLE handle_;
    DWORD id_;
};

namespace this_thread {
    template <class Rep, class Period>
    void sleep_for(const std::chrono::duration<Rep, Period>& sleep_duration) {
        auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(sleep_duration).count();
        if (ms > 0) {
            Sleep(static_cast<DWORD>(ms));
        } else if (sleep_duration.count() > 0) {
            Sleep(1);
        }
    }
}

} // namespace std
