#ifndef HAUVIE_DIAGNOSTIC_H
#define HAUVIE_DIAGNOSTIC_H

#include "hauvie.h"
#include "hauvie_receiver.h"
#include "hauvie_token.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    PipelineReport pipeline;
    bool receiver_analysis_requested;
    HauvieReceiverComparison receiver_comparison;
} HauvieDiagnosticReport;

/* Run the patient diagnostic and, when requested, finite receiver comparison. */
PipelineResult hauvie_diagnostic_run(
    const HauvieInput *input,
    HauvieDiagnosticReport *report
);

#ifdef __cplusplus
}
#endif

#endif
