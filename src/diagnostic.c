#include "hauvie_diagnostic.h"

#include <stddef.h>

PipelineResult hauvie_diagnostic_run(
    const HauvieInput *input,
    HauvieDiagnosticReport *report
) {
    ExtruderContext context;

    if (report == NULL) {
        return PIPELINE_INVALID_ARGUMENT;
    }
    *report = (HauvieDiagnosticReport){
        .pipeline = {.result = PIPELINE_INVALID_ARGUMENT},
        .receiver_analysis_requested = false,
        .receiver_comparison = HAUVIE_RECEIVER_INVALID
    };
    if (input == NULL) {
        return PIPELINE_INVALID_ARGUMENT;
    }

    report->receiver_analysis_requested = input->receiver_analysis_enabled;
    report->pipeline.result = hauvies_helper_run(
        &context, &input->patient, &report->pipeline);
    if (report->pipeline.result != PIPELINE_OK &&
        report->pipeline.result != PIPELINE_CRITICAL_HAUVIES) {
        return report->pipeline.result;
    }

    if (input->receiver_analysis_enabled) {
        HauvieReceiverSample left = {
            .context_id = input->receiver_context_id,
            .output_count = 1,
            .outputs = {input->receiver_left_output}
        };
        HauvieReceiverSample right = {
            .context_id = input->receiver_context_id,
            .output_count = 1,
            .outputs = {input->receiver_right_output}
        };
        report->receiver_comparison = hauvie_receiver_compare(
            &left, &right, input->receiver_tolerance);
        if (report->receiver_comparison == HAUVIE_RECEIVER_INVALID) {
            return PIPELINE_INVALID_ARGUMENT;
        }
    }
    return report->pipeline.result;
}
