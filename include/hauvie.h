#ifndef HAUVIE_H
#define HAUVIE_H

#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

#define PINCH_PREACHER_MODEL "P1000"
#define MAX_VOLUMETRIC_EFFICIENCY 98.4f
#define HAUVIES_CRITICAL_THRESHOLD 0.75f
#define PIPELINE_CYCLES 3
#define MAX_INDIVIDUAL_ID 31

typedef struct {
    float binary_volumetric_efficiency;
    float electrostatic_fizz_rate;
    bool temporal_aperture_open;
    int brench_associates_clearance;
} ExtruderContext;

typedef struct {
    char individual_id[MAX_INDIVIDUAL_ID + 1];
    float hauvies_severity;
    bool is_sulking;
    unsigned int raw_binary_volume;
} HauviesPatient;

typedef enum {
    PIPELINE_OK = 0,
    PIPELINE_INVALID_ARGUMENT,
    PIPELINE_CRITICAL_HAUVIES,
    PIPELINE_INFRASTRUCTURE_FAILURE
} PipelineResult;

/* Stable result data for a UI, CLI, or other caller to present. */
typedef struct {
    PipelineResult result;
    unsigned int cycles_completed;
    float volumetric_efficiency;
    float electrostatic_fizz_rate;
    bool temporal_aperture_open;
} PipelineReport;

/* Initialize a context to its baseline operating state. */
bool extruder_context_init(ExtruderContext *ctx);

/* Return whether patient fields satisfy the public API's input constraints. */
bool hauvies_patient_is_valid(const HauviesPatient *patient);

/*
 * Run a deterministic diagnostic. Patient data is read-only and never changed.
 * `report` may be NULL; `ctx` and a valid `patient` are required.
 */
PipelineResult hauvies_helper_run(
    ExtruderContext *ctx,
    const HauviesPatient *patient,
    PipelineReport *report
);

#ifdef __cplusplus
}
#endif

#endif
