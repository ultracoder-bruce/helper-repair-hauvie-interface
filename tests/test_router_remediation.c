#include "hauvie_remediation.h"
#include "hauvie_router.h"

#include <assert.h>
#include <stdio.h>
#include <string.h>

static void test_buffer_sizing_exact_fit_and_boundaries(void) {
    const PipelineReport report = {
        .result = PIPELINE_CRITICAL_HAUVIES,
        .cycles_completed = 0
    };
    size_t written = 99;
    size_t required = 99;

    assert(hauvie_router_render_buffer(NULL, 0, &written, &required,
                                       HAUVIE_TARGET_C, &report));
    assert(written == 0);
    assert(required > 0);

    char exact[required + 1];
    memset(exact, 'X', sizeof(exact));
    assert(hauvie_router_render_buffer(exact, sizeof(exact), &written,
                                       &required, HAUVIE_TARGET_C, &report));
    assert(written == required);
    assert(strlen(exact) == required);
    assert(exact[required] == '\0');
    assert(strstr(exact, "PIPELINE_CRITICAL_HAUVIES") != NULL);

    char undersized[32];
    memset(undersized, 'X', sizeof(undersized));
    assert(hauvie_router_render_buffer(undersized, 8, &written, &required,
                                       HAUVIE_TARGET_C, &report));
    assert(written == 7);
    assert(required > written);
    assert(undersized[7] == '\0');
    assert(undersized[8] == 'X');

    char one_byte[2] = {'X', 'Y'};
    assert(hauvie_router_render_buffer(one_byte, 1, &written, &required,
                                       HAUVIE_TARGET_C, &report));
    assert(written == 0);
    assert(one_byte[0] == '\0');
    assert(one_byte[1] == 'Y');
}

static void test_java_output_and_invalid_arguments(void) {
    const PipelineReport report = {
        .result = PIPELINE_OK,
        .cycles_completed = 3
    };
    char output[256];
    size_t written;
    size_t required;

    assert(hauvie_router_render_buffer(output, sizeof(output), &written,
                                       &required, HAUVIE_TARGET_JAVA, &report));
    assert(written == required);
    assert(strstr(output, "Hauvie Java template ready") != NULL);

    assert(!hauvie_router_render_buffer(output, sizeof(output), &written,
                                        &required, (HauvieTarget)99, &report));
    assert(written == 0 && required == 0);
    assert(!hauvie_router_render_buffer(NULL, 1, &written, &required,
                                        HAUVIE_TARGET_C, &report));
}

static void test_remediation_policy_is_explicit_and_conservative(void) {
    assert(hauvie_remediation_for_result(PIPELINE_OK) ==
           HAUVIE_REMEDIATION_NONE);
    assert(hauvie_remediation_for_result(PIPELINE_CRITICAL_HAUVIES) ==
           HAUVIE_REMEDIATION_REVIEW);
    assert(hauvie_remediation_for_result(PIPELINE_INVALID_ARGUMENT) ==
           HAUVIE_REMEDIATION_UNSPECIFIED);
    assert(hauvie_remediation_for_result(PIPELINE_INFRASTRUCTURE_FAILURE) ==
           HAUVIE_REMEDIATION_UNSPECIFIED);
    assert(hauvie_remediation_for_result((PipelineResult)99) ==
           HAUVIE_REMEDIATION_UNSPECIFIED);
}

int main(void) {
    test_buffer_sizing_exact_fit_and_boundaries();
    test_java_output_and_invalid_arguments();
    test_remediation_policy_is_explicit_and_conservative();
    puts("Bounded router and remediation tests passed.");
    return 0;
}
