#include "hauvie_token_stream.h"
#include "hauvie_token.h"
#include "hauvie.h"
#include "hauvie_router.h"

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

static void test_ast_state_transitions(void) {
    HauvieAst ast;
    assert(hauvie_parse_ast(
        "patient=Tech-Agent-404 severity=0.9 sulking=false target=java", &ast));
    assert(ast.status == HAUVIE_AST_VERIFIED);
    assert(ast.error == HAUVIE_PARSE_ERROR_NONE);
    assert(ast.input.patient.hauvies_severity > HAUVIES_CRITICAL_THRESHOLD);
    assert(ast.input.target == HAUVIE_TARGET_JAVA);

    ExtruderContext ctx;
    PipelineReport report;
    assert(hauvies_helper_run(&ctx, &ast.input.patient, &report) == PIPELINE_CRITICAL_HAUVIES);

    assert(!hauvie_parse_ast("patient=x;", &ast));
    assert(ast.status == HAUVIE_AST_REJECTED);
    assert(ast.error == HAUVIE_PARSE_ERROR_INVALID_INPUT);
    assert(ast.input.patient.individual_id[0] != '\0');
}

static void test_token_stream(void) {
    HauvieTokenStream stream;
    HauvieToken token;
    assert(hauvie_token_stream_init(&stream, "patient=alpha severity=0.3"));
    assert(hauvie_token_stream_next(&stream, &token) == HAUVIE_STREAM_TOKEN);
    assert(token.kind == HAUVIE_TOKEN_WORD && token.length == 7);
    assert(memcmp(token.text, "patient", 7) == 0 && token.offset == 0);
    assert(hauvie_token_stream_next(&stream, &token) == HAUVIE_STREAM_TOKEN);
    assert(token.kind == HAUVIE_TOKEN_EQUALS && token.offset == 7);
    assert(hauvie_token_stream_next(&stream, &token) == HAUVIE_STREAM_TOKEN);
    assert(token.kind == HAUVIE_TOKEN_WORD && token.length == 5);
    assert(memcmp(token.text, "alpha", 5) == 0);
    assert(hauvie_token_stream_next(&stream, &token) == HAUVIE_STREAM_TOKEN);
    assert(token.kind == HAUVIE_TOKEN_WORD && token.offset == 13);
    assert(hauvie_token_stream_next(&stream, &token) == HAUVIE_STREAM_TOKEN);
    assert(token.kind == HAUVIE_TOKEN_EQUALS);
    assert(hauvie_token_stream_next(&stream, &token) == HAUVIE_STREAM_TOKEN);
    assert(token.length == 3 && memcmp(token.text, "0.3", 3) == 0);
    assert(hauvie_token_stream_next(&stream, &token) == HAUVIE_STREAM_END);
}

static void test_stream_limits_and_parser_integration(void) {
    HauvieTokenStream stream;
    HauvieToken token;
    char too_many_tokens[128];
    size_t used = 0;
    for (size_t i = 0; i < 11; ++i) {
        int written = snprintf(too_many_tokens + used,
            sizeof(too_many_tokens) - used, "%sa=b ", i == 0 ? "" : "");
        assert(written > 0);
        used += (size_t)written;
    }
    assert(hauvie_token_stream_init(&stream, too_many_tokens));
    HauvieStreamResult result;
    do {
        result = hauvie_token_stream_next(&stream, &token);
    } while (result == HAUVIE_STREAM_TOKEN);
    assert(result == HAUVIE_STREAM_ERROR);

    HauvieInput input;
    assert(hauvie_parse_input("patient=streamed severity=0.4 target=java", &input));
    assert(strcmp(input.patient.individual_id, "streamed") == 0);
    assert(input.target == HAUVIE_TARGET_JAVA);
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
    test_ast_state_transitions();
    test_token_stream();
    test_stream_limits_and_parser_integration();
    test_token_ast_survives_critical_severity();
    test_token_parser_rejects_bad_syntax();
    puts("Core Hauvie API and token stream tests passed.");
    return 0;
}
