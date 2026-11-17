#include "hauvie_repair_queue.h"

#include <string.h>

static bool queue_submit(void *context, HauvieFsmProvider provider,
                         uint64_t request_id, const void *request_data,
                         size_t request_size) {
    HauvieRepairQueueEntry *entry = context;
    HauvieRepairQueue *queue = entry->queue;
    return queue->submit(queue->context, provider, request_id, request_data,
                         request_size);
}

static void queue_cancel(void *context, HauvieFsmProvider provider,
                         uint64_t request_id) {
    HauvieRepairQueueEntry *entry = context;
    HauvieRepairQueue *queue = entry->queue;
    if (queue->cancel != NULL) {
        queue->cancel(queue->context, provider, request_id);
    }
}

static void queue_terminal(void *context, HauvieFsmAction action) {
    HauvieRepairQueueEntry *entry = context;
    entry->terminal_action = action;
    entry->terminal_pending = true;
}

static void finish_active(HauvieRepairQueue *queue) {
    if (queue == NULL || !queue->active || queue->count == 0) {
        return;
    }

    HauvieRepairQueueEntry *entry = &queue->entries[queue->head];
    if (!entry->terminal_pending) {
        return;
    }

    uint64_t job_id = entry->job_id;
    HauvieFsmAction terminal_action = entry->terminal_action;
    entry->occupied = false;
    entry->terminal_pending = false;
    entry->request_size = 0;
    queue->head = (queue->head + 1u) % HAUVIE_REPAIR_QUEUE_CAPACITY;
    --queue->count;
    queue->active = false;

    if (queue->on_terminal != NULL) {
        queue->on_terminal(queue->context, job_id, terminal_action);
    }
}

bool hauvie_repair_queue_init(
    HauvieRepairQueue *queue,
    HauvieRuntimeSubmitFn submit,
    HauvieRuntimeCancelFn cancel,
    HauvieRepairTerminalFn on_terminal,
    void *context
) {
    if (queue == NULL || submit == NULL) {
        return false;
    }
    memset(queue, 0, sizeof(*queue));
    queue->submit = submit;
    queue->cancel = cancel;
    queue->on_terminal = on_terminal;
    queue->context = context;
    return true;
}

bool hauvie_repair_queue_enqueue(
    HauvieRepairQueue *queue,
    uint64_t job_id,
    const void *request_data,
    size_t request_size
) {
    if (queue == NULL || queue->submit == NULL ||
        (request_data == NULL && request_size != 0) ||
        request_size > HAUVIE_REPAIR_QUEUE_MAX_REQUEST_SIZE ||
        queue->count >= HAUVIE_REPAIR_QUEUE_CAPACITY) {
        return false;
    }

    size_t index = (queue->head + queue->count) % HAUVIE_REPAIR_QUEUE_CAPACITY;
    HauvieRepairQueueEntry *entry = &queue->entries[index];
    memset(entry, 0, sizeof(*entry));
    entry->queue = queue;
    entry->slot_index = index;
    entry->job_id = job_id;
    entry->request_size = request_size;
    entry->occupied = true;
    if (request_size != 0) {
        memcpy(entry->request_data, request_data, request_size);
    }
    ++queue->count;
    return true;
}

bool hauvie_repair_queue_pump(HauvieRepairQueue *queue) {
    if (queue == NULL || queue->submit == NULL) {
        return false;
    }
    if (queue->active) {
        return true;
    }
    if (queue->count == 0) {
        return false;
    }

    HauvieRepairQueueEntry *entry = &queue->entries[queue->head];
    if (!entry->occupied || !hauvie_runtime_init(
            &entry->runtime, queue_submit, queue_cancel, queue_terminal, entry,
            entry->request_size == 0 ? NULL : entry->request_data,
            entry->request_size)) {
        entry->terminal_action = (HauvieFsmAction){
            .type = HAUVIE_FSM_ACTION_FAILED,
            .provider = HAUVIE_FSM_ALPHA,
            .request_id = 0
        };
        entry->terminal_pending = true;
        queue->active = true;
        finish_active(queue);
        return queue->count != 0;
    }

    queue->active = true;
    if (!hauvie_runtime_start(&entry->runtime) && !entry->terminal_pending) {
        entry->terminal_action = (HauvieFsmAction){
            .type = HAUVIE_FSM_ACTION_FAILED,
            .provider = HAUVIE_FSM_ALPHA,
            .request_id = 0
        };
        entry->terminal_pending = true;
    }
    finish_active(queue);
    return queue->active || queue->count != 0;
}

void hauvie_repair_queue_complete(
    HauvieRepairQueue *queue,
    HauvieFsmProvider provider,
    uint64_t request_id,
    HauvieFsmResult result
) {
    if (queue == NULL || !queue->active) {
        return;
    }
    HauvieRepairQueueEntry *entry = &queue->entries[queue->head];
    hauvie_runtime_complete(&entry->runtime, provider, request_id, result);
    finish_active(queue);
}

void hauvie_repair_queue_timeout(
    HauvieRepairQueue *queue,
    HauvieFsmProvider provider,
    uint64_t request_id
) {
    if (queue == NULL || !queue->active) {
        return;
    }
    HauvieRepairQueueEntry *entry = &queue->entries[queue->head];
    hauvie_runtime_timeout(&entry->runtime, provider, request_id);
    finish_active(queue);
}

void hauvie_repair_queue_cancel(HauvieRepairQueue *queue) {
    if (queue == NULL || !queue->active) {
        return;
    }
    HauvieRepairQueueEntry *entry = &queue->entries[queue->head];
    hauvie_runtime_cancel(&entry->runtime);
    finish_active(queue);
}

size_t hauvie_repair_queue_pending(const HauvieRepairQueue *queue) {
    return queue != NULL ? queue->count : 0;
}

bool hauvie_repair_queue_is_active(const HauvieRepairQueue *queue) {
    return queue != NULL && queue->active;
}
