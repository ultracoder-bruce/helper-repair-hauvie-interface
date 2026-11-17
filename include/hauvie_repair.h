#ifndef HAUVIE_REPAIR_H
#define HAUVIE_REPAIR_H

#include "hauvie_diagnostic.h"
#include "hauvie_repair_queue.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    HAUVIE_REPAIR_ENQUEUED = 0,
    HAUVIE_REPAIR_NOT_REQUIRED,
    HAUVIE_REPAIR_INVALID_INPUT,
    HAUVIE_REPAIR_QUEUE_FULL,
    HAUVIE_REPAIR_REQUEST_TOO_LARGE
} HauvieRepairEnqueueResult;

/*
 * Enqueue a bounded review request only for a critical diagnostic. This does
 * not perform a repair; provider callbacks decide what external work to do.
 */
HauvieRepairEnqueueResult hauvie_repair_enqueue_diagnostic(
    HauvieRepairQueue *queue,
    uint64_t job_id,
    const HauvieInput *input,
    const HauvieDiagnosticReport *diagnostic
);

#ifdef __cplusplus
}
#endif

#endif
