// Vita3K iOS embedded runtime entrypoints for XMB Manic.
// Copyright (C) 2026 Vita3K contributors; GPL-2.0-or-later.
// The full emulator is built into Vita3KManicRuntime.framework.
// These are synchronous C functions; do not call run from a background queue.
#ifndef VITA3K_MANIC_RUNTIME_H
#define VITA3K_MANIC_RUNTIME_H
#include <stddef.h>
#ifdef __cplusplus
extern "C" {
#endif

// Presents the Vita3K native window/library using the caller's existing
// UIApplication. Must be called on the iOS MAIN thread. This call pumps
// UIKit's run loop and returns when the Vita window closes. Returns 0 on
// normal exit; a negative value on initialization or threading failure.
int manic_vita3k_run(void);

// Requests graceful termination of the current Vita session/library.
void manic_vita3k_request_exit(void);

// Queue a file selected by the XMB app's own Files picker before starting
// Vita3K. Kind 1 = official firmware PUP, kind 2 = game VPK/ZIP/PKG.
// The path must refer to a durable app-owned copy. Returns 1 when queued.
int manic_vita3k_enqueue_import(const char *path, int kind);

// Called from the embedded Vita frontend when no import is in progress.
// Returns 1 with a path and kind, 0 for an empty queue, -1 if the buffer is
// too small (the queued entry is preserved).
int manic_vita3k_take_import(char *path, size_t capacity, int *kind);

// Nonzero while the embedded runtime is active.
int manic_vita3k_is_running(void);

#ifdef __cplusplus
}
#endif
#endif
