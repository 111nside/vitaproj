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
#include <cstring>
#include <deque>
#include <mutex>
#include <string>
#include <pthread.h>

int vita3k_manic_embedded_entry(int argc, char *argv[]);

namespace {
std::atomic_bool g_runtime_running{false};
struct PendingImport {
    std::string path;
    int kind;
};
std::mutex g_pending_import_mutex;
std::deque<PendingImport> g_pending_imports;


struct RunningScope {
    ~RunningScope() { g_runtime_running.store(false, std::memory_order_release); }
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
    // In the standalone application SDL3's main shim does this at startup.
    // Manic already owns UIApplicationMain and must instead call this directly.
    SDL_SetMainReady();
    char app_name[] = "Vita3KManicRuntime";
    char *argv[] = {app_name, nullptr};
    return vita3k_manic_embedded_entry(1, argv);
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

extern "C" int manic_vita3k_is_running(void) {
    return g_runtime_running.load(std::memory_order_acquire) ? 1 : 0;
}
