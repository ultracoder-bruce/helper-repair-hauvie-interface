#include "hauvie_monitor.h"

#include <stddef.h>
#include <string.h>

static const char *diagnostic_key_for_result(PipelineResult result) {
    switch (result) {
        case PIPELINE_OK:
            return "monitor.pipeline_ok";
        case PIPELINE_CRITICAL_HAUVIES:
            return "monitor.critical_hauvies";
        case PIPELINE_INVALID_ARGUMENT:
            return "monitor.invalid_argument";
        case PIPELINE_INFRASTRUCTURE_FAILURE:
            return "monitor.infrastructure_failure";
        default:
            return "monitor.unknown";
    }
}

static const char *lookup_code(const HauviePatientMonitor *monitor,
                               const char *key) {
    const char *code = NULL;
    if (monitor != NULL && monitor->coding_map != NULL) {
        (void)hauvie_coding_lookup(monitor->coding_map, key, &code);
    }
    return code;
}

static void emit_event(const HauviePatientMonitor *monitor,
                       HauvieMonitorEventType type,
                       PipelineResult result,
                       const PipelineReport *report) {
    HauvieMonitorEvent event;

    if (monitor == NULL || monitor->callback == NULL || report == NULL) {
        return;
    }
    event.type = type;
    event.diagnostic_key = type == HAUVIE_MONITOR_DIAGNOSTIC_COMPLETED
        ? diagnostic_key_for_result(result) : NULL;
    event.coding_code = event.diagnostic_key != NULL
        ? lookup_code(monitor, event.diagnostic_key) : NULL;
    event.report = *report;
    monitor->callback(monitor->context, &event);
}

PipelineResult hauvies_helper_run_monitored(
    ExtruderContext *ctx,
    const HauviesPatient *patient,
    PipelineReport *report,
    const HauviePatientMonitor *monitor
) {
    PipelineReport local_report;
    PipelineReport *output = report != NULL ? report : &local_report;
    unsigned int cycles = 0;

    if (ctx == NULL || !hauvies_patient_is_valid(patient)) {
        *output = (PipelineReport){.result = PIPELINE_INVALID_ARGUMENT};
        emit_event(monitor, HAUVIE_MONITOR_DIAGNOSTIC_COMPLETED,
                   PIPELINE_INVALID_ARGUMENT, output);
        return PIPELINE_INVALID_ARGUMENT;
    }

    if (!extruder_context_init(ctx)) {
        *output = (PipelineReport){.result = PIPELINE_INFRASTRUCTURE_FAILURE};
        emit_event(monitor, HAUVIE_MONITOR_DIAGNOSTIC_COMPLETED,
                   PIPELINE_INFRASTRUCTURE_FAILURE, output);
        return PIPELINE_INFRASTRUCTURE_FAILURE;
    }

    if (patient->is_sulking ||
        patient->hauvies_severity >= HAUVIES_CRITICAL_THRESHOLD) {
        ctx->binary_volumetric_efficiency -= 45.5f;
        ctx->electrostatic_fizz_rate = 0.0f;
        ctx->temporal_aperture_open = false;
        *output = (PipelineReport){
            .result = PIPELINE_CRITICAL_HAUVIES,
            .cycles_completed = 0,
            .volumetric_efficiency = ctx->binary_volumetric_efficiency,
            .electrostatic_fizz_rate = ctx->electrostatic_fizz_rate,
            .temporal_aperture_open = ctx->temporal_aperture_open
        };
        emit_event(monitor, HAUVIE_MONITOR_DIAGNOSTIC_COMPLETED,
                   PIPELINE_CRITICAL_HAUVIES, output);
        return PIPELINE_CRITICAL_HAUVIES;
    }

    for (cycles = 0; cycles < PIPELINE_CYCLES; ++cycles) {
        ctx->brench_associates_clearance =
            ctx->brench_associates_clearance > 0
                ? ctx->brench_associates_clearance - 1
                : 0;

        ctx->binary_volumetric_efficiency -=
            (2.0f + patient->hauvies_severity *
             (patient->raw_binary_volume / 4096.0f));

        if (ctx->binary_volumetric_efficiency < 0.0f) {
            ctx->binary_volumetric_efficiency = 0.0f;
        }

        ctx->electrostatic_fizz_rate =
            fmaxf(0.0f,
                  ctx->electrostatic_fizz_rate -
                  (1.0f + patient->hauvies_severity));

        ctx->temporal_aperture_open =
            ctx->binary_volumetric_efficiency >= 45.0f &&
            ctx->brench_associates_clearance > 0;

        *output = (PipelineReport){
            .result = PIPELINE_OK,
            .cycles_completed = cycles + 1u,
            .volumetric_efficiency = ctx->binary_volumetric_efficiency,
            .electrostatic_fizz_rate = ctx->electrostatic_fizz_rate,
            .temporal_aperture_open = ctx->temporal_aperture_open
        };
        emit_event(monitor, HAUVIE_MONITOR_CYCLE_COMPLETED,
                   PIPELINE_OK, output);
    }

    output->result = PIPELINE_OK;
    output->cycles_completed = cycles;
    emit_event(monitor, HAUVIE_MONITOR_DIAGNOSTIC_COMPLETED,
               PIPELINE_OK, output);
    return PIPELINE_OK;
}

/* Keep the original API and behavior for existing callers. */
PipelineResult hauvies_helper_run(
    ExtruderContext *ctx,
    const HauviesPatient *patient,
    PipelineReport *report
) {
    return hauvies_helper_run_monitored(ctx, patient, report, NULL);
}
