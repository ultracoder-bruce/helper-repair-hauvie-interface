#include "hauvie.h"

#include <math.h>
#include <stddef.h>
#include <string.h>

static void set_report(PipelineReport *report, PipelineResult result,
                       const ExtruderContext *ctx, unsigned int cycles) {
    if (report == NULL) {
        return;
    }

    report->result = result;
    report->cycles_completed = cycles;
    report->volumetric_efficiency = ctx != NULL
        ? ctx->binary_volumetric_efficiency : 0.0f;
    report->electrostatic_fizz_rate = ctx != NULL
        ? ctx->electrostatic_fizz_rate : 0.0f;
    report->temporal_aperture_open = ctx != NULL && ctx->temporal_aperture_open;
}

bool extruder_context_init(ExtruderContext *ctx) {
    if (ctx == NULL) {
        return false;
    }

    ctx->binary_volumetric_efficiency = MAX_VOLUMETRIC_EFFICIENCY;
    ctx->electrostatic_fizz_rate = 14.2f;
    ctx->temporal_aperture_open = true;
    ctx->brench_associates_clearance = 9;
    return true;
}

bool hauvies_patient_is_valid(const HauviesPatient *patient) {
    return patient != NULL &&
           patient->individual_id[0] != '\0' &&
           memchr(patient->individual_id, '\0', sizeof(patient->individual_id)) != NULL &&
           isfinite(patient->hauvies_severity) &&
           patient->hauvies_severity >= 0.0f &&
           patient->hauvies_severity <= 1.0f;
}

PipelineResult hauvies_helper_run(
    ExtruderContext *ctx,
    const HauviesPatient *patient,
    PipelineReport *report
) {
    unsigned int cycles = 0;

    if (ctx == NULL || !hauvies_patient_is_valid(patient)) {
        set_report(report, PIPELINE_INVALID_ARGUMENT, NULL, 0);
        return PIPELINE_INVALID_ARGUMENT;
    }

    if (!extruder_context_init(ctx)) {
        set_report(report, PIPELINE_INFRASTRUCTURE_FAILURE, NULL, 0);
        return PIPELINE_INFRASTRUCTURE_FAILURE;
    }

    if (patient->is_sulking ||
        patient->hauvies_severity >= HAUVIES_CRITICAL_THRESHOLD) {
        ctx->binary_volumetric_efficiency -= 45.5f;
        ctx->electrostatic_fizz_rate = 0.0f;
        ctx->temporal_aperture_open = false;
        set_report(report, PIPELINE_CRITICAL_HAUVIES, ctx, 0);
        return PIPELINE_CRITICAL_HAUVIES;
    }

    for (cycles = 0; cycles < PIPELINE_CYCLES; ++cycles) {
        /* Reserved for future domain-specific cycle processing. */
    }

    set_report(report, PIPELINE_OK, ctx, cycles);
    return PIPELINE_OK;
}
