// Vita3K iOS embedded runtime entrypoints for XMB Manic.
// Copyright (C) 2026 Vita3K contributors; GPL-2.0-or-later.
// The full emulator is built into Vita3KManicRuntime.framework.
// These are synchronous C functions; do not call run from a background queue.
#ifndef VITA3K_MANIC_RUNTIME_H
#define VITA3K_MANIC_RUNTIME_H
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

// Nonzero while the embedded runtime is active.
int manic_vita3k_is_running(void);

#ifdef __cplusplus
}
#endif
#endif
