#include "hauvie.h"
#include "hauvie_diagnostic.h"
#include "hauvie_router.h"
#include "hauvie_token.h"

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

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
               report->volumetric_efficiency, MAX_VOLUMETRIC_EFFICIENCY);
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

static const char *receiver_status_name(HauvieReceiverComparison comparison) {
    switch (comparison) {
        case HAUVIE_RECEIVER_EQUIVALENT:
            return "equivalent under the supplied context and tolerance";
        case HAUVIE_RECEIVER_DISTINGUISHABLE:
            return "distinguishable under the supplied context and tolerance";
        case HAUVIE_RECEIVER_CONTEXT_MISMATCH:
            return "context mismatch";
        case HAUVIE_RECEIVER_INVALID:
        default:
            return "invalid receiver analysis input";
    }
}

static int run_diagnostic(const HauvieInput *input, bool route_output) {
    HauvieDiagnosticReport diagnostic;
    PipelineResult result = hauvie_diagnostic_run(input, &diagnostic);

    if (result == PIPELINE_INVALID_ARGUMENT ||
        result == PIPELINE_INFRASTRUCTURE_FAILURE) {
        fprintf(stderr, "[ERROR] Hauvie helper could not run (result %d).\n", result);
        return EXIT_FAILURE;
    }

    if (route_output) {
        if (!hauvie_router_render(stdout, input->target, &diagnostic.pipeline)) {
            fprintf(stderr, "[ERROR] Could not route output template.\n");
            return EXIT_FAILURE;
        }
    } else {
        print_report(&input->patient, &diagnostic.pipeline);
    }

    if (diagnostic.receiver_analysis_requested) {
        printf("[RECEIVER] %s\n",
               receiver_status_name(diagnostic.receiver_comparison));
    }
    return result == PIPELINE_OK ? EXIT_SUCCESS : EXIT_FAILURE;
}

static HauvieInput default_input(void) {
    return (HauvieInput){
        .patient = {
            .individual_id = "Tech-Agent-404",
            .hauvies_severity = 0.2f,
            .is_sulking = false,
            .raw_binary_volume = 4096
        },
        .target = HAUVIE_TARGET_C,
        .receiver_analysis_enabled = false
    };
}

static bool parse_cli_severity(const char *text, float *severity) {
    char *end = NULL;
    float value;

    if (text == NULL || severity == NULL || text[0] == '\0') {
        return false;
    }
    errno = 0;
    value = strtof(text, &end);
    if (errno == ERANGE || end == text || *end != '\0' || value < 0.0f ||
        value > 1.0f) {
        return false;
    }
    *severity = value;
    return true;
}

int main(int argc, char **argv) {
    HauvieInput input = default_input();

    if (argc == 3 && strcmp(argv[1], "--source") == 0) {
        HauvieAst ast;
        if (!hauvie_parse_ast(argv[2], &ast)) {
            fprintf(stderr,
                    "[ERROR] Input rejected during AST verification (state=%d, error=%d).\n",
                    ast.status, ast.error);
            return EXIT_FAILURE;
        }
        if (ast.status != HAUVIE_AST_VERIFIED) {
            fprintf(stderr, "[ERROR] Parser returned an unexpected AST state.\n");
            return EXIT_FAILURE;
        }
        return run_diagnostic(&ast.input, true);
    }

    if (argc > 3) {
        fprintf(stderr,
                "Usage: %s [patient-id [severity]] | --source 'patient=... severity=... sulking=false target=c|java [receiver_context=N receiver_left=0..1 receiver_right=0..1 receiver_tolerance=N]'\n",
                argv[0]);
        return EXIT_FAILURE;
    }
    if (argc >= 2) {
        size_t id_length = strlen(argv[1]);
        if (id_length == 0 || id_length > MAX_INDIVIDUAL_ID) {
            fprintf(stderr, "[ERROR] Patient ID must be 1-%d characters.\n",
                    MAX_INDIVIDUAL_ID);
            return EXIT_FAILURE;
        }
        memcpy(input.patient.individual_id, argv[1], id_length + 1);
    }
    if (argc == 3 && !parse_cli_severity(argv[2],
                                         &input.patient.hauvies_severity)) {
        fprintf(stderr, "[ERROR] Severity must be a number from 0.0 to 1.0.\n");
        return EXIT_FAILURE;
    }
    return run_diagnostic(&input, false);
}
