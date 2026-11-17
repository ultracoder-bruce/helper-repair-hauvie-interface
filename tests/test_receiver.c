#include "hauvie_receiver.h"

#include <assert.h>
#include <math.h>
#include <stdio.h>

static void test_equivalence_uses_absolute_tolerance(void) {
    HauvieReceiverSample left = {
        .context_id = 4,
        .output_count = 2,
        .outputs = {0.25, 0.75}
    };
    HauvieReceiverSample nearby = {
        .context_id = 4,
        .output_count = 2,
        .outputs = {0.2505, 0.7495}
    };
    HauvieReceiverSample distinguishable = {
        .context_id = 4,
        .output_count = 2,
        .outputs = {0.26, 0.74}
    };

    assert(hauvie_receiver_compare(&left, &nearby, 0.001) ==
           HAUVIE_RECEIVER_EQUIVALENT);
    assert(hauvie_receiver_compare(&left, &distinguishable, 0.001) ==
           HAUVIE_RECEIVER_DISTINGUISHABLE);
}

static void test_contexts_are_not_mixed(void) {
    HauvieReceiverSample left = {
        .context_id = 1,
        .output_count = 1,
        .outputs = {0.5}
    };
    HauvieReceiverSample other_context = {
        .context_id = 2,
        .output_count = 1,
        .outputs = {0.5}
    };

    assert(hauvie_receiver_compare(&left, &other_context, 0.0) ==
           HAUVIE_RECEIVER_CONTEXT_MISMATCH);
}

static void test_invalid_samples_and_tolerances(void) {
    HauvieReceiverSample valid = {
        .context_id = 0,
        .output_count = 1,
        .outputs = {0.5}
    };
    HauvieReceiverSample empty = {0};
    HauvieReceiverSample non_finite = {
        .context_id = 0,
        .output_count = 1,
        .outputs = {NAN}
    };
    HauvieReceiverSample too_many = {
        .context_id = 0,
        .output_count = HAUVIE_RECEIVER_MAX_OUTPUTS + 1u,
        .outputs = {0.0}
    };

    assert(hauvie_receiver_sample_is_valid(&valid));
    assert(!hauvie_receiver_sample_is_valid(&empty));
    assert(!hauvie_receiver_sample_is_valid(&non_finite));
    assert(!hauvie_receiver_sample_is_valid(&too_many));
    assert(hauvie_receiver_compare(NULL, &valid, 0.0) == HAUVIE_RECEIVER_INVALID);
    assert(hauvie_receiver_compare(&valid, &valid, -1.0) == HAUVIE_RECEIVER_INVALID);
    assert(hauvie_receiver_compare(&valid, &valid, INFINITY) == HAUVIE_RECEIVER_INVALID);
}

int main(void) {
    test_equivalence_uses_absolute_tolerance();
    test_contexts_are_not_mixed();
    test_invalid_samples_and_tolerances();
    puts("Receiver model tests passed.");
    return 0;
}
