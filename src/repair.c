#include "hauvie_repair.h"

#include <stdio.h>
#include <string.h>

HauvieRepairEnqueueResult hauvie_repair_enqueue_diagnostic(
    HauvieRepairQueue *queue,
    uint64_t job_id,
    const HauvieInput *input,
    const HauvieDiagnosticReport *diagnostic
) {
    char request[HAUVIE_REPAIR_QUEUE_MAX_REQUEST_SIZE];
    const char *receiver = "not-requested";
    int written;

    if (queue == NULL || input == NULL || diagnostic == NULL ||
        !hauvies_patient_is_valid(&input->patient)) {
        return HAUVIE_REPAIR_INVALID_INPUT;
    }
    if (diagnostic->pipeline.result != PIPELINE_CRITICAL_HAUVIES) {
        return HAUVIE_REPAIR_NOT_REQUIRED;
    }

    if (diagnostic->receiver_analysis_requested) {
        switch (diagnostic->receiver_comparison) {
            case HAUVIE_RECEIVER_EQUIVALENT:
                receiver = "equivalent";
                break;
            case HAUVIE_RECEIVER_DISTINGUISHABLE:
                receiver = "distinguishable";
                break;
            case HAUVIE_RECEIVER_CONTEXT_MISMATCH:
                receiver = "context-mismatch";
                break;
            case HAUVIE_RECEIVER_INVALID:
            default:
                return HAUVIE_REPAIR_INVALID_INPUT;
        }
    }

    written = snprintf(request, sizeof(request),
        "action=review patient=%s severity=%.9g volumetric_efficiency=%.9g "
        "electrostatic_fizz_rate=%.9g temporal_aperture_open=%s receiver=%s",
        input->patient.individual_id,
        (double)input->patient.hauvies_severity,
        (double)diagnostic->pipeline.volumetric_efficiency,
        (double)diagnostic->pipeline.electrostatic_fizz_rate,
        diagnostic->pipeline.temporal_aperture_open ? "true" : "false",
        receiver);
    if (written < 0 || (size_t)written >= sizeof(request)) {
        return HAUVIE_REPAIR_REQUEST_TOO_LARGE;
    }
    if (!hauvie_repair_queue_enqueue(queue, job_id, request,
                                     (size_t)written + 1u)) {
        return hauvie_repair_queue_pending(queue) >=
                HAUVIE_REPAIR_QUEUE_CAPACITY
            ? HAUVIE_REPAIR_QUEUE_FULL : HAUVIE_REPAIR_INVALID_INPUT;
    }
    return HAUVIE_REPAIR_ENQUEUED;
}
