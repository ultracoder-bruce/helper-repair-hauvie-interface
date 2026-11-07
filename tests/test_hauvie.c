#include "hauvie.h"

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

int main(void) {
    test_successful_run();
    test_critical_run();
    test_invalid_patients();
    puts("Core Hauvie API tests passed.");
    return 0;
}
