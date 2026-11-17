#ifndef HAUVIE_RECEIVER_H
#define HAUVIE_RECEIVER_H

#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Receiver results are bounded and independent of the patient DSL. */
#define HAUVIE_RECEIVER_MAX_OUTPUTS 8

typedef struct {
    unsigned int context_id;
    size_t output_count;
    double outputs[HAUVIE_RECEIVER_MAX_OUTPUTS];
} HauvieReceiverSample;

typedef enum {
    HAUVIE_RECEIVER_INVALID = 0,
    HAUVIE_RECEIVER_EQUIVALENT,
    HAUVIE_RECEIVER_DISTINGUISHABLE,
    HAUVIE_RECEIVER_CONTEXT_MISMATCH
} HauvieReceiverComparison;

/* Return whether a sample has a supported number of finite outputs. */
bool hauvie_receiver_sample_is_valid(const HauvieReceiverSample *sample);

/*
 * Compare outputs from the same receiver context using absolute tolerance.
 * Invalid samples, a non-finite tolerance, or a negative tolerance produce
 * HAUVIE_RECEIVER_INVALID. Different contexts are never treated as equivalent.
 */
HauvieReceiverComparison hauvie_receiver_compare(
    const HauvieReceiverSample *left,
    const HauvieReceiverSample *right,
    double absolute_tolerance
);

#ifdef __cplusplus
}
#endif

#endif
