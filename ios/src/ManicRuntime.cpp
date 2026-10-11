// Vita3K native host adapter for XMB Manic.
// SPDX-License-Identifier: GPL-2.0-or-later
//
// UpstreamMain.cpp's standalone main() is compiled by CMake with a per-source
// symbol rename ONLY for the embedded target. No second UIApplicationMain or
// SDL_main entrypoint is linked into Manic.
// This file lives inside Manic's existing UIApplication. Prevent SDL_main.h
// from generating a second native main() shim that references SDL_main.
// The standalone Tsubomi entrypoint continues to use SDL's normal startup.
#define SDL_MAIN_HANDLED 1
#include <vita3k_ios/ManicRuntime.h>
#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>
#include <atomic>
#include <algorithm>
#include <cstring>
#include <deque>
#include <mutex>
#include <string>
#include <pthread.h>

int vita3k_manic_embedded_entry(int argc, char *argv[]);

namespace {
std::atomic_bool g_runtime_running{false};
std::atomic_int g_host_mode{0};
std::string g_next_title;
std::mutex g_next_title_mutex;
struct PendingImport {
    std::string path;
    int kind;
};
std::mutex g_pending_import_mutex;
std::deque<PendingImport> g_pending_imports;
std::mutex g_import_result_mutex;
std::string g_import_result_message;
int g_import_result_ok = 0;


struct RunningScope {
    ~RunningScope() {
        g_host_mode.store(0, std::memory_order_release);
        g_runtime_running.store(false, std::memory_order_release);
    }
};
} // namespace

extern "C" int manic_vita3k_run(void) {
    if (!pthread_main_np()) {
        SDL_Log("Manic Vita3K: embedded runtime must run on the UIKit main thread");
        return -2;
    }
    if (g_runtime_running.exchange(true, std::memory_order_acq_rel))
        return -3;

    RunningScope reset;
    {
        std::lock_guard<std::mutex> lock(g_import_result_mutex);
        g_import_result_message.clear();
        g_import_result_ok = 0;
    }
    // In the standalone application SDL3's main shim does this at startup.
    // Manic already owns UIApplicationMain and must instead call this directly.
    SDL_SetMainReady();
    char app_name[] = "Vita3KManicRuntime";
    char *argv[] = {app_name, nullptr};
    manic_vita3k_capture_host_window();
    const int status = vita3k_manic_embedded_entry(1, argv);
    manic_vita3k_restore_host_window();
    return status;
}

extern "C" void manic_vita3k_request_exit(void) {
    if (!g_runtime_running.load(std::memory_order_acquire)
        || !(SDL_WasInit(SDL_INIT_VIDEO) & SDL_INIT_VIDEO))
        return;
    SDL_Event quit_event{};
    quit_event.type = SDL_EVENT_QUIT;
    SDL_PushEvent(&quit_event);
}

extern "C" int manic_vita3k_enqueue_import(const char *path, int kind) {
    if (!path || !*path || std::strlen(path) > 4096 || (kind != 1 && kind != 2))
        return 0;
    std::lock_guard<std::mutex> lock(g_pending_import_mutex);
    if (g_pending_imports.size() >= 16)
        return 0;
    g_pending_imports.push_back({path, kind});
    return 1;
}

extern "C" int manic_vita3k_take_import(char *path, size_t capacity, int *kind) {
    if (!path || !kind || capacity == 0)
        return -1;
    std::lock_guard<std::mutex> lock(g_pending_import_mutex);
    if (g_pending_imports.empty())
        return 0;
    const PendingImport &entry = g_pending_imports.front();
    if (entry.path.size() + 1 > capacity)
        return -1;
    std::memcpy(path, entry.path.c_str(), entry.path.size() + 1);
    *kind = entry.kind;
    g_pending_imports.pop_front();
    return 1;
}

extern "C" void manic_vita3k_set_host_mode(int mode) {
    if (g_runtime_running.load(std::memory_order_acquire))
        return;
    g_host_mode.store(mode >= 0 && mode <= 2 ? mode : 0, std::memory_order_release);
}

extern "C" int manic_vita3k_host_mode(void) {
    return g_host_mode.load(std::memory_order_acquire);
}

extern "C" int manic_vita3k_queue_title(const char *title_id) {
    if (!title_id || !*title_id)
        return 0;
    const size_t length = std::strlen(title_id);
    if (length > 16)
        return 0;
    for (size_t i = 0; i < length; ++i) {
        const unsigned char ch = static_cast<unsigned char>(title_id[i]);
        if (!((ch >= 'A' && ch <= 'Z') || (ch >= 'a' && ch <= 'z')
                || (ch >= '0' && ch <= '9') || ch == '_' || ch == '-'))
            return 0;
    }
    std::lock_guard<std::mutex> lock(g_next_title_mutex);
    g_next_title = title_id;
    return 1;
}

extern "C" int manic_vita3k_take_title(char *title_id, size_t capacity) {
    if (!title_id || capacity == 0)
        return -1;
    std::lock_guard<std::mutex> lock(g_next_title_mutex);
    if (g_next_title.empty())
        return 0;
    if (g_next_title.size() + 1 > capacity)
        return -1;
    std::memcpy(title_id, g_next_title.c_str(), g_next_title.size() + 1);
    g_next_title.clear();
    return 1;
}

extern "C" int manic_vita3k_pending_import_count(void) {
    std::lock_guard<std::mutex> lock(g_pending_import_mutex);
    return static_cast<int>(g_pending_imports.size());
}

extern "C" void manic_vita3k_record_import_result(const char *message, int success) {
    std::lock_guard<std::mutex> lock(g_import_result_mutex);
    if (!success)
        g_import_result_ok = -1;
    else if (g_import_result_ok == 0)
        g_import_result_ok = 1;
    if (message && *message) {
        if (!g_import_result_message.empty())
            g_import_result_message += "\n";
        g_import_result_message += message;
    }
}

extern "C" int manic_vita3k_last_import_ok(void) {
    std::lock_guard<std::mutex> lock(g_import_result_mutex);
    return g_import_result_ok == 1 ? 1 : 0;
}

extern "C" int manic_vita3k_copy_import_result(char *message, size_t capacity) {
    if (!message || capacity == 0)
        return 0;
    std::lock_guard<std::mutex> lock(g_import_result_mutex);
    const size_t n = std::min(g_import_result_message.size(), capacity - 1);
    std::memcpy(message, g_import_result_message.data(), n);
    message[n] = '\0';
    return static_cast<int>(n);
}

extern "C" int manic_vita3k_is_running(void) {
    return g_runtime_running.load(std::memory_order_acquire) ? 1 : 0;
}
