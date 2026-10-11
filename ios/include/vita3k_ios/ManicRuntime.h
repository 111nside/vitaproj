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

// Keep the original Manic UIWindow available so direct game sessions restore
// normal XMB touch input after SDL destroys its temporary window.
void manic_vita3k_capture_host_window(void);
void manic_vita3k_restore_host_window(void);

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

// Run the embedded core as a short installation job (1), a direct title
// session (2), or its legacy internal library (0). The XMB host owns library
// navigation so UIKit can complete button and document-picker transactions.
// Must be configured before manic_vita3k_run on the main thread.
void manic_vita3k_set_host_mode(int mode);
int manic_vita3k_host_mode(void);

// Select a game by installed Vita title ID before a direct-title run.
// Returns 1 for a safe queued identifier; 0 for an invalid identifier.
int manic_vita3k_queue_title(const char *title_id);
int manic_vita3k_take_title(char *title_id, size_t capacity);

// Number of staged files waiting for the next installation run.
int manic_vita3k_pending_import_count(void);

// Install outcome reported by the embedded core and shown by Manic after
// control has returned to its normal UIKit event loop.
void manic_vita3k_record_import_result(const char *message, int success);
int manic_vita3k_last_import_ok(void);
int manic_vita3k_copy_import_result(char *message, size_t capacity);

// Nonzero while the embedded runtime is active.
int manic_vita3k_is_running(void);

#ifdef __cplusplus
}
#endif
#endif
