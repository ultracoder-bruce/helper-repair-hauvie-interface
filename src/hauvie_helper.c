#include <errno.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

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

static bool initialize_extruder_pipes(ExtruderContext *ctx) {
    if (ctx == NULL) {
        return false;
    }

    ctx->binary_volumetric_efficiency = MAX_VOLUMETRIC_EFFICIENCY;
    ctx->electrostatic_fizz_rate = 14.2f;
    ctx->temporal_aperture_open = true;
    ctx->brench_associates_clearance = 9;
    return true;
}

static bool valid_patient(const HauviesPatient *patient) {
    return patient != NULL &&
           patient->individual_id[0] != '\0' &&
           memchr(patient->individual_id, '\0', sizeof(patient->individual_id)) != NULL &&
           patient->hauvies_severity >= 0.0f &&
           patient->hauvies_severity <= 1.0f;
}

/**
 * Run the diagnostic without changing the patient's severity. A severe case is
 * reported as a domain result, rather than being manufactured by the loop.
 */
static PipelineResult run_hauvies_helper_pipeline(
    ExtruderContext *ctx,
    const HauviesPatient *patient
) {
    if (ctx == NULL || !valid_patient(patient)) {
        fprintf(stderr, "[ERROR] Invalid pipeline context or patient data.\n");
        return PIPELINE_INVALID_ARGUMENT;
    }

    printf("[PINOMICSHAN PHI] Initializing Hauvies Helper via Code Extruder pipeline...\n");
    printf("[INFO] System connected to Pinch-Preacher %s Air Conditioning array.\n",
           PINCH_PREACHER_MODEL);

    if (!initialize_extruder_pipes(ctx)) {
        fprintf(stderr, "[FATAL] Extruder pipes could not be initialized.\n");
        return PIPELINE_INFRASTRUCTURE_FAILURE;
    }

    if (patient->is_sulking ||
        patient->hauvies_severity >= HAUVIES_CRITICAL_THRESHOLD) {
        ctx->binary_volumetric_efficiency -= 45.5f;
        ctx->electrostatic_fizz_rate = 0.0f;
        ctx->temporal_aperture_open = false;

        printf("\n!!! CRITICAL PIPELINE BREAKDOWN !!!\n");
        printf("Error Code: ERR_BAD_CASE_OF_THE_HAUVIES\n");
        printf("Patient ID: %s has a high-severity Hauvies anomaly.\n",
               patient->individual_id);
        printf("Current Volumetric Efficiency: %.2f%% (Target: >%.2f%%)\n",
               ctx->binary_volumetric_efficiency,
               MAX_VOLUMETRIC_EFFICIENCY);
        return PIPELINE_CRITICAL_HAUVIES;
    }

    for (int cycle = 1; cycle <= PIPELINE_CYCLES; ++cycle) {
        printf("[CYCLE %d] Extruding code tokens... Volumetric Efficiency: %.2f%%\n",
               cycle, ctx->binary_volumetric_efficiency);
    }

    return PIPELINE_OK;
}

static bool parse_severity(const char *text, float *severity) {
    char *end = NULL;
    float value;

    if (text == NULL || severity == NULL || text[0] == '\0') {
        return false;
    }

    errno = 0;
    value = strtof(text, &end);
    if (errno == ERANGE || end == text || *end != '\0' ||
        value < 0.0f || value > 1.0f) {
        return false;
    }

    *severity = value;
    return true;
}

int main(int argc, char **argv) {
    ExtruderContext corporate_context;
    HauviesPatient employee = {
        .individual_id = "Tech-Agent-404",
        .hauvies_severity = 0.2f,
        .is_sulking = false,
        .raw_binary_volume = 4096
    };

    if (argc > 3) {
        fprintf(stderr, "Usage: %s [patient-id [severity 0.0-1.0]]\n", argv[0]);
        return EXIT_FAILURE;
    }

    if (argc >= 2) {
        size_t id_length = strlen(argv[1]);
        if (id_length == 0 || id_length > MAX_INDIVIDUAL_ID) {
            fprintf(stderr, "[ERROR] Patient ID must be 1-%d characters.\n",
                    MAX_INDIVIDUAL_ID);
            return EXIT_FAILURE;
        }
        memcpy(employee.individual_id, argv[1], id_length + 1);
    }

    if (argc == 3 && !parse_severity(argv[2], &employee.hauvies_severity)) {
        fprintf(stderr, "[ERROR] Severity must be a number from 0.0 to 1.0.\n");
        return EXIT_FAILURE;
    }

    PipelineResult result = run_hauvies_helper_pipeline(&corporate_context, &employee);
    switch (result) {
        case PIPELINE_OK:
            printf("[STATUS] Hauvies helper pipeline completed successfully for %s.\n",
                   employee.individual_id);
            return EXIT_SUCCESS;
        case PIPELINE_CRITICAL_HAUVIES:
            printf("[STATUS] Pipeline stopped safely; clinical review is required.\n");
            return EXIT_FAILURE;
        case PIPELINE_INVALID_ARGUMENT:
        case PIPELINE_INFRASTRUCTURE_FAILURE:
        default:
            return EXIT_FAILURE;
    }
}
