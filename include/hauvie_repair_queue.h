#ifndef HAUVIE_REPAIR_QUEUE_H
#define HAUVIE_REPAIR_QUEUE_H

#include "hauvie_runtime.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define HAUVIE_REPAIR_QUEUE_CAPACITY 8u
#define HAUVIE_REPAIR_QUEUE_MAX_REQUEST_SIZE HAUVIE_RUNTIME_MAX_REQUEST_SIZE

typedef void (*HauvieRepairTerminalFn)(
    void *context,
    uint64_t job_id,
    HauvieFsmAction terminal_action
);

typedef struct HauvieRepairQueue HauvieRepairQueue;

typedef struct {
    HauvieRuntime runtime;
    HauvieRepairQueue *queue;
    size_t slot_index;
    uint64_t job_id;
    unsigned char request_data[HAUVIE_REPAIR_QUEUE_MAX_REQUEST_SIZE];
    size_t request_size;
    HauvieFsmAction terminal_action;
    bool occupied;
    bool terminal_pending;
} HauvieRepairQueueEntry;

/* Caller-owned, bounded FIFO. All API calls must be serialized on one thread. */
struct HauvieRepairQueue {
    HauvieRepairQueueEntry entries[HAUVIE_REPAIR_QUEUE_CAPACITY];
    size_t head;
    size_t count;
    bool active;
    HauvieRuntimeSubmitFn submit;
    HauvieRuntimeCancelFn cancel;
    HauvieRepairTerminalFn on_terminal;
    void *context;
};

/* Initialize an empty queue with provider callbacks shared by all requests. */
bool hauvie_repair_queue_init(
    HauvieRepairQueue *queue,
    HauvieRuntimeSubmitFn submit,
    HauvieRuntimeCancelFn cancel,
    HauvieRepairTerminalFn on_terminal,
    void *context
);

/* Copy a bounded request into the FIFO; its caller may then release the data. */
bool hauvie_repair_queue_enqueue(
    HauvieRepairQueue *queue,
    uint64_t job_id,
    const void *request_data,
    size_t request_size
);

/* Start the FIFO head if idle; returns true while a request is active. */
bool hauvie_repair_queue_pump(HauvieRepairQueue *queue);

/* Deliver provider events to the active FIFO head. Stale IDs are ignored. */
void hauvie_repair_queue_complete(
    HauvieRepairQueue *queue,
    HauvieFsmProvider provider,
    uint64_t request_id,
    HauvieFsmResult result
);
void hauvie_repair_queue_timeout(
    HauvieRepairQueue *queue,
    HauvieFsmProvider provider,
    uint64_t request_id
);
void hauvie_repair_queue_cancel(HauvieRepairQueue *queue);

size_t hauvie_repair_queue_pending(const HauvieRepairQueue *queue);
bool hauvie_repair_queue_is_active(const HauvieRepairQueue *queue);

#ifdef __cplusplus
}
#endif

#endif
