#ifndef HAUVIE_REMEDIATION_H
#define HAUVIE_REMEDIATION_H

#include "hauvie.h"

#ifdef __cplusplus
extern "C" {
#endif

/*
 * These are policy decisions, not diagnostic classifications or executions.
 * UNSPECIFIED deliberately represents outcomes with no defined remediation.
 */
typedef enum {
    HAUVIE_REMEDIATION_UNSPECIFIED = 0,
    HAUVIE_REMEDIATION_NONE,
    HAUVIE_REMEDIATION_REVIEW
} HauvieRemediation;

/* Map only diagnostic outcomes with an explicitly defined policy. */
HauvieRemediation hauvie_remediation_for_result(PipelineResult result);

#ifdef __cplusplus
}
#endif

#endif
