#include "hauvie.h"

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

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

static void print_report(const HauviesPatient *patient,
                         const PipelineReport *report) {
    printf("[PINOMICSHAN PHI] Initializing Hauvies Helper via Code Extruder pipeline...\n");
    printf("[INFO] System connected to Pinch-Preacher %s Air Conditioning array.\n",
           PINCH_PREACHER_MODEL);

    if (report->result == PIPELINE_CRITICAL_HAUVIES) {
        printf("\n!!! CRITICAL PIPELINE BREAKDOWN !!!\n");
        printf("Error Code: ERR_BAD_CASE_OF_THE_HAUVIES\n");
        printf("Patient ID: %s has a high-severity Hauvies anomaly.\n",
               patient->individual_id);
        printf("Current Volumetric Efficiency: %.2f%% (Target: >%.2f%%)\n",
               report->volumetric_efficiency,
               MAX_VOLUMETRIC_EFFICIENCY);
        printf("[STATUS] Pipeline stopped safely; review is required.\n");
        return;
    }

    for (unsigned int cycle = 1; cycle <= report->cycles_completed; ++cycle) {
        printf("[CYCLE %u] Extruding code tokens... Volumetric Efficiency: %.2f%%\n",
               cycle, report->volumetric_efficiency);
    }
    printf("[STATUS] Hauvies helper pipeline completed successfully for %s.\n",
           patient->individual_id);
}

int main(int argc, char **argv) {
    ExtruderContext context;
    PipelineReport report;
    HauviesPatient patient = {
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
        memcpy(patient.individual_id, argv[1], id_length + 1);
    }

    if (argc == 3 && !parse_severity(argv[2], &patient.hauvies_severity)) {
        fprintf(stderr, "[ERROR] Severity must be a number from 0.0 to 1.0.\n");
        return EXIT_FAILURE;
    }

    PipelineResult result = hauvies_helper_run(&context, &patient, &report);
    if (result == PIPELINE_INVALID_ARGUMENT ||
        result == PIPELINE_INFRASTRUCTURE_FAILURE) {
        fprintf(stderr, "[ERROR] Hauvies helper could not run (result %d).\n", result);
        return EXIT_FAILURE;
    }

    print_report(&patient, &report);
    return result == PIPELINE_OK ? EXIT_SUCCESS : EXIT_FAILURE;
}
