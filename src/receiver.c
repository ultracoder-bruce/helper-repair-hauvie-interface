#include "hauvie_receiver.h"

#include <math.h>

bool hauvie_receiver_sample_is_valid(const HauvieReceiverSample *sample) {
    if (sample == NULL || sample->output_count == 0 ||
        sample->output_count > HAUVIE_RECEIVER_MAX_OUTPUTS) {
        return false;
    }

    for (size_t i = 0; i < sample->output_count; ++i) {
        if (!isfinite(sample->outputs[i])) {
            return false;
        }
    }
    return true;
}

HauvieReceiverComparison hauvie_receiver_compare(
    const HauvieReceiverSample *left,
    const HauvieReceiverSample *right,
    double absolute_tolerance
) {
    if (!hauvie_receiver_sample_is_valid(left) ||
        !hauvie_receiver_sample_is_valid(right) ||
        !isfinite(absolute_tolerance) || absolute_tolerance < 0.0) {
        return HAUVIE_RECEIVER_INVALID;
    }

    if (left->context_id != right->context_id) {
        return HAUVIE_RECEIVER_CONTEXT_MISMATCH;
    }
    if (left->output_count != right->output_count) {
        return HAUVIE_RECEIVER_DISTINGUISHABLE;
    }

    for (size_t i = 0; i < left->output_count; ++i) {
        if (fabs(left->outputs[i] - right->outputs[i]) > absolute_tolerance) {
            return HAUVIE_RECEIVER_DISTINGUISHABLE;
        }
    }
    return HAUVIE_RECEIVER_EQUIVALENT;
}
