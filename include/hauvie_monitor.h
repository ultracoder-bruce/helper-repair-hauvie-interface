#ifndef HAUVIE_MONITOR_H
#define HAUVIE_MONITOR_H

#include "hauvie.h"
#include "hauvie_coding.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    HAUVIE_MONITOR_CYCLE_COMPLETED = 0,
    HAUVIE_MONITOR_DIAGNOSTIC_COMPLETED
} HauvieMonitorEventType;

typedef struct {
    HauvieMonitorEventType type;
    /* Set for diagnostic events; NULL for cycle progress events. */
    const char *diagnostic_key;
    /* Configured code, or NULL when no mapping is loaded for this key. */
    const char *coding_code;
    /* For diagnostic events this is the final result; cycle reports are snapshots. */
    PipelineReport report;
} HauvieMonitorEvent;

/*
 * The callback is synchronous and must be fast, nonblocking, and non-reentrant.
 * Copy any event data needed after return. Events intentionally omit patient
 * identifiers and raw patient fields; applications can enrich them only under
 * their own privacy, consent, and transport policies.
 */
typedef void (*HauvieMonitorCallback)(
    void *context,
    const HauvieMonitorEvent *event
);

typedef struct {
    HauvieMonitorCallback callback;
    void *context;
    /* Optional caller-owned, previously loaded ICD-10-CM mapping. */
    const HauvieCodingMap *coding_map;
} HauviePatientMonitor;

/*
 * Run the existing deterministic patient pipeline, optionally reporting each
 * completed loop cycle and one final diagnostic event. Callback delivery does
 * not change the result of the diagnostic pipeline.
 */
PipelineResult hauvies_helper_run_monitored(
    ExtruderContext *ctx,
    const HauviesPatient *patient,
    PipelineReport *report,
    const HauviePatientMonitor *monitor
);

#ifdef __cplusplus
}
#endif

#endif
