#include "hauvie_diagnostic.h"
#include "hauvie_token.h"

#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <string.h>

static HauvieDiagnosticReport run_source(const char *source, HauvieInput *parsed) {
    HauvieDiagnosticReport report;
    assert(hauvie_parse_input(source, parsed));
    PipelineResult result = hauvie_diagnostic_run(parsed, &report);
    assert(result == report.pipeline.result);
    return report;
}

static void test_receiver_comparison_is_independent_of_patient_fields(void) {
    HauvieInput input_a;
    HauvieInput input_b;
    HauvieDiagnosticReport report_a = run_source(
        "patient=alpha severity=0.2 receiver_context=7 receiver_left=0.45 receiver_right=0.4505 receiver_tolerance=0.001",
        &input_a);
    HauvieDiagnosticReport report_b = run_source(
        "patient=beta severity=0.4 receiver_context=7 receiver_left=0.45 receiver_right=0.4505 receiver_tolerance=0.001",
        &input_b);

    assert(strcmp(input_a.patient.individual_id, "alpha") == 0);
    assert(strcmp(input_b.patient.individual_id, "beta") == 0);
    assert(report_a.receiver_analysis_requested && report_b.receiver_analysis_requested);
    assert(report_a.receiver_comparison == HAUVIE_RECEIVER_EQUIVALENT);
    assert(report_b.receiver_comparison == HAUVIE_RECEIVER_EQUIVALENT);
    assert(report_a.pipeline.result == PIPELINE_OK);
    assert(report_b.pipeline.result == PIPELINE_OK);
}

static void test_receiver_distinguishability_and_diagnostic_impact(void) {
    HauvieInput input;
    HauvieDiagnosticReport report = run_source(
        "patient=critical severity=0.8 sulking=false target=java receiver_context=2 receiver_left=0.45 receiver_right=0.5 receiver_tolerance=0.01",
        &input);

    assert(report.pipeline.result == PIPELINE_CRITICAL_HAUVIES);
    assert(report.receiver_analysis_requested);
    assert(report.receiver_comparison == HAUVIE_RECEIVER_DISTINGUISHABLE);
    assert(report.pipeline.cycles_completed == 0);
    assert(!report.pipeline.temporal_aperture_open);
    assert(input.target == HAUVIE_TARGET_JAVA);
}

static void test_receiver_fields_are_all_or_none(void) {
    HauvieInput input;
    assert(!hauvie_parse_input(
        "patient=alpha receiver_context=1 receiver_left=0.2", &input));
    assert(!hauvie_parse_input(
        "patient=alpha receiver_context=1 receiver_left=0.2 receiver_right=0.3 receiver_tolerance=-0.1",
        &input));
    assert(!hauvie_parse_input(
        "patient=alpha receiver_context=1 receiver_left=0.2 receiver_right=1.1 receiver_tolerance=0.01",
        &input));
}

static void test_legacy_patient_source_has_no_receiver_analysis(void) {
    HauvieInput input;
    HauvieDiagnosticReport report = run_source(
        "patient=ordinary severity=0.2 target=c", &input);
    assert(!report.receiver_analysis_requested);
    assert(report.receiver_comparison == HAUVIE_RECEIVER_INVALID);
    assert(report.pipeline.result == PIPELINE_OK);
    assert(isfinite(report.pipeline.volumetric_efficiency));
}

int main(void) {
    test_receiver_comparison_is_independent_of_patient_fields();
    test_receiver_distinguishability_and_diagnostic_impact();
    test_receiver_fields_are_all_or_none();
    test_legacy_patient_source_has_no_receiver_analysis();
    puts("Integrated receiver diagnostic tests passed.");
    return 0;
}
