#include "hauvie.h"
#include "hauvie_router.h"
#include "hauvie_token.h"

#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <string.h>

static void test_successful_run(void) {
    ExtruderContext ctx;
    PipelineReport report;
    HauviesPatient patient = {
        .individual_id = "test-patient",
        .hauvies_severity = 0.2f,
        .is_sulking = false,
        .raw_binary_volume = 4096
    };
    HauviesPatient original = patient;

    assert(hauvies_helper_run(&ctx, &patient, &report) == PIPELINE_OK);
    assert(report.cycles_completed == PIPELINE_CYCLES);
    assert(report.temporal_aperture_open);
    assert(fabsf(report.volumetric_efficiency - MAX_VOLUMETRIC_EFFICIENCY) < 0.001f);
    assert(memcmp(&patient, &original, sizeof(patient)) == 0);
}

static void test_critical_run(void) {
    ExtruderContext ctx;
    PipelineReport report;
    HauviesPatient patient = {
        .individual_id = "critical-patient",
        .hauvies_severity = HAUVIES_CRITICAL_THRESHOLD,
        .is_sulking = false,
        .raw_binary_volume = 4096
    };

    assert(hauvies_helper_run(&ctx, &patient, &report) == PIPELINE_CRITICAL_HAUVIES);
    assert(report.cycles_completed == 0);
    assert(!report.temporal_aperture_open);
    assert(report.electrostatic_fizz_rate == 0.0f);
}

static void test_invalid_patients(void) {
    ExtruderContext ctx;
    PipelineReport report;
    HauviesPatient patient = {
        .individual_id = "bad-severity",
        .hauvies_severity = NAN,
        .is_sulking = false,
        .raw_binary_volume = 0
    };

    assert(hauvies_helper_run(&ctx, &patient, &report) == PIPELINE_INVALID_ARGUMENT);
    patient.hauvies_severity = 1.1f;
    assert(!hauvies_patient_is_valid(&patient));
    assert(hauvies_helper_run(&ctx, NULL, &report) == PIPELINE_INVALID_ARGUMENT);
    assert(hauvies_helper_run(NULL, &patient, &report) == PIPELINE_INVALID_ARGUMENT);
}

static void test_token_ast_survives_critical_severity(void) {
    HauvieInput input;
    assert(hauvie_parse_input(
        "patient=Tech-Agent-404 severity=0.9 sulking=false target=java", &input));
    assert(strcmp(input.patient.individual_id, "Tech-Agent-404") == 0);
    assert(input.patient.hauvies_severity > HAUVIES_CRITICAL_THRESHOLD);
    assert(input.target == HAUVIE_TARGET_JAVA);

    ExtruderContext ctx;
    PipelineReport report;
    assert(hauvies_helper_run(&ctx, &input.patient, &report) == PIPELINE_CRITICAL_HAUVIES);
    assert(hauvie_router_render(stdout, input.target, &report));
}

static void test_token_parser_rejects_bad_syntax(void) {
    HauvieInput input;
    assert(!hauvie_parse_input("patient= severity=0.2", &input));
    assert(!hauvie_parse_input("severity=nan", &input));
    assert(!hauvie_parse_input("patient=x patient=y", &input));
    assert(!hauvie_parse_input("target=python", &input));
    assert(!hauvie_parse_input("patient=x;", &input));
}

int main(void) {
    test_successful_run();
    test_critical_run();
    test_invalid_patients();
    test_token_ast_survives_critical_severity();
    test_token_parser_rejects_bad_syntax();
    puts("Core Hauvie API tests passed.");
    return 0;
}
