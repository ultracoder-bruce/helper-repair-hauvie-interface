#ifndef HAUVIE_RUNTIME_H
#define HAUVIE_RUNTIME_H

#include "hauvie_fsm.h"

#include <stddef.h>
#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Hard upper bound for a borrowed provider request payload. */
#define HAUVIE_RUNTIME_MAX_REQUEST_SIZE 512u

typedef bool (*HauvieRuntimeSubmitFn)(
    void *context,
    HauvieFsmProvider provider,
    uint64_t request_id,
    const void *request_data,
    size_t request_size
);

typedef void (*HauvieRuntimeCancelFn)(
    void *context,
    HauvieFsmProvider provider,
    uint64_t request_id
);

typedef void (*HauvieRuntimeTerminalFn)(
    void *context,
    HauvieFsmAction terminal_action
);

typedef struct {
    HauvieFsm fsm;
    HauvieRuntimeSubmitFn submit;
    HauvieRuntimeCancelFn cancel;
    HauvieRuntimeTerminalFn on_terminal;
    void *context;
    const void *request_data;
    size_t request_size;
    bool initialized;
} HauvieRuntime;

/*
 * Initialize a caller-owned runtime. Payloads are limited to
 * HAUVIE_RUNTIME_MAX_REQUEST_SIZE bytes; NULL is allowed only for a zero-size
 * payload. The payload is borrowed, must remain alive and unchanged through a
 * terminal callback, and must be readable for request_size bytes.
 *
 * Callbacks must enqueue/perform only bounded nonblocking work, must not call
 * runtime APIs reentrantly, and must not retain the payload beyond the
 * terminal callback unless they make their own copy. The runtime is not
 * thread-safe: serialize all API calls and callbacks on one event-loop thread.
 *
 * A submit callback returning false means the request was not accepted and is
 * treated as a transport failure, triggering Alpha-to-Beta failover when
 * applicable. See docs/runtime-api.md for the full callback and lifecycle
 * contract.
 */
bool hauvie_runtime_init(
    HauvieRuntime *runtime,
    HauvieRuntimeSubmitFn submit,
    HauvieRuntimeCancelFn cancel,
    HauvieRuntimeTerminalFn on_terminal,
    void *context,
    const void *request_data,
    size_t request_size
);

/* Start Alpha. True means the runtime accepted the start operation. */
bool hauvie_runtime_start(HauvieRuntime *runtime);

/* Deliver a provider completion; stale provider/request IDs are ignored. */
void hauvie_runtime_complete(
    HauvieRuntime *runtime,
    HauvieFsmProvider provider,
    uint64_t request_id,
    HauvieFsmResult result
);

/* Deliver a timeout for a request; stale timeout events are ignored. */
void hauvie_runtime_timeout(
    HauvieRuntime *runtime,
    HauvieFsmProvider provider,
    uint64_t request_id
);

/* Cancel the currently pending request, if any. */
void hauvie_runtime_cancel(HauvieRuntime *runtime);

#ifdef __cplusplus
}
#endif

#endif
