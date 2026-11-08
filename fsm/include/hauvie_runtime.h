#ifndef HAUVIE_RUNTIME_H
#define HAUVIE_RUNTIME_H

#include "hauvie_fsm.h"

#include <stddef.h>
#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

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
 * Initialize a caller-owned runtime. Request data is borrowed and must remain
 * valid until a terminal action is delivered. The submit callback should
 * enqueue work and return promptly; completions are delivered later through
 * hauvie_runtime_complete(). Returning false means the request was not
 * accepted and is treated as a transport failure.
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
