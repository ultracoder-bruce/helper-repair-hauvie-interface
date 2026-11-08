#include "hauvie_runtime.h"

#include <assert.h>
#include <stdio.h>
#include <string.h>

typedef struct {
    HauvieFsmProvider providers[8];
    uint64_t request_ids[8];
    size_t submitted;
    HauvieFsmProvider cancelled_providers[8];
    uint64_t cancelled_ids[8];
    size_t cancelled;
    HauvieFsmAction terminal;
    size_t terminal_count;
    HauvieFsmProvider rejected_provider;
} FakeProvider;

static bool fake_submit(void *context, HauvieFsmProvider provider,
                        uint64_t request_id, const void *data, size_t size) {
    FakeProvider *fake = context;
    assert(fake->submitted < 8);
    assert(data != NULL);
    assert(size == strlen("request"));
    assert(memcmp(data, "request", size) == 0);
    fake->providers[fake->submitted] = provider;
    fake->request_ids[fake->submitted] = request_id;
    ++fake->submitted;
    return provider != fake->rejected_provider;
}

static void fake_cancel(void *context, HauvieFsmProvider provider,
                        uint64_t request_id) {
    FakeProvider *fake = context;
    assert(fake->cancelled < 8);
    fake->cancelled_providers[fake->cancelled] = provider;
    fake->cancelled_ids[fake->cancelled] = request_id;
    ++fake->cancelled;
}

static void fake_terminal(void *context, HauvieFsmAction action) {
    FakeProvider *fake = context;
    fake->terminal = action;
    ++fake->terminal_count;
}

static void init_runtime(HauvieRuntime *runtime, FakeProvider *fake) {
    fake->rejected_provider = (HauvieFsmProvider)-1;
    assert(hauvie_runtime_init(runtime, fake_submit, fake_cancel,
                               fake_terminal, fake, "request",
                               strlen("request")));
}

static void test_init_enforces_payload_bounds(void) {
    HauvieRuntime runtime;
    FakeProvider fake = {0};
    char payload[HAUVIE_RUNTIME_MAX_REQUEST_SIZE] = {0};

    assert(!hauvie_runtime_init(&runtime, fake_submit, fake_cancel,
                                fake_terminal, &fake, NULL, 1));
    assert(!hauvie_runtime_init(&runtime, fake_submit, fake_cancel,
                                fake_terminal, &fake, payload,
                                HAUVIE_RUNTIME_MAX_REQUEST_SIZE + 1u));
    assert(hauvie_runtime_init(&runtime, fake_submit, fake_cancel,
                               fake_terminal, &fake, NULL, 0));
    assert(hauvie_runtime_init(&runtime, fake_submit, fake_cancel,
                               fake_terminal, &fake, payload,
                               HAUVIE_RUNTIME_MAX_REQUEST_SIZE));
}

static void test_success_and_stale_completion(void) {
    HauvieRuntime runtime;
    FakeProvider fake = {0};
    init_runtime(&runtime, &fake);
    assert(hauvie_runtime_start(&runtime));
    assert(fake.submitted == 1);
    assert(fake.providers[0] == HAUVIE_FSM_ALPHA);

    hauvie_runtime_complete(&runtime, HAUVIE_FSM_ALPHA,
                            fake.request_ids[0] + 1,
                            HAUVIE_FSM_RESULT_VALID);
    assert(fake.terminal_count == 0);
    assert(runtime.fsm.state == HAUVIE_FSM_ALPHA_PENDING);

    hauvie_runtime_complete(&runtime, HAUVIE_FSM_ALPHA,
                            fake.request_ids[0], HAUVIE_FSM_RESULT_VALID);
    assert(fake.terminal_count == 1);
    assert(fake.terminal.type == HAUVIE_FSM_ACTION_READY);
    assert(runtime.fsm.state == HAUVIE_FSM_VERIFIED);
}

static void test_alpha_failover_beta_success_and_late_alpha(void) {
    HauvieRuntime runtime;
    FakeProvider fake = {0};
    init_runtime(&runtime, &fake);
    assert(hauvie_runtime_start(&runtime));
    uint64_t alpha_id = fake.request_ids[0];

    hauvie_runtime_complete(&runtime, HAUVIE_FSM_ALPHA, alpha_id,
                            HAUVIE_FSM_RESULT_TRANSPORT_ERROR);
    assert(fake.submitted == 2);
    assert(fake.providers[1] == HAUVIE_FSM_BETA);
    assert(fake.request_ids[1] != alpha_id);

    hauvie_runtime_complete(&runtime, HAUVIE_FSM_ALPHA, alpha_id,
                            HAUVIE_FSM_RESULT_VALID);
    assert(fake.terminal_count == 0);
    assert(runtime.fsm.state == HAUVIE_FSM_BETA_PENDING);

    hauvie_runtime_complete(&runtime, HAUVIE_FSM_BETA, fake.request_ids[1],
                            HAUVIE_FSM_RESULT_VALID);
    assert(fake.terminal_count == 1);
    assert(fake.terminal.type == HAUVIE_FSM_ACTION_READY);
}

static void test_timeout_cancels_and_fails_over(void) {
    HauvieRuntime runtime;
    FakeProvider fake = {0};
    init_runtime(&runtime, &fake);
    assert(hauvie_runtime_start(&runtime));
    uint64_t alpha_id = fake.request_ids[0];

    hauvie_runtime_timeout(&runtime, HAUVIE_FSM_ALPHA, alpha_id + 1);
    assert(fake.cancelled == 0);
    assert(fake.submitted == 1);

    hauvie_runtime_timeout(&runtime, HAUVIE_FSM_ALPHA, alpha_id);
    assert(fake.cancelled == 1);
    assert(fake.cancelled_providers[0] == HAUVIE_FSM_ALPHA);
    assert(fake.cancelled_ids[0] == alpha_id);
    assert(fake.submitted == 2);
    assert(fake.providers[1] == HAUVIE_FSM_BETA);

    uint64_t beta_id = fake.request_ids[1];
    hauvie_runtime_timeout(&runtime, HAUVIE_FSM_BETA, beta_id);
    assert(fake.cancelled == 2);
    assert(fake.terminal_count == 1);
    assert(fake.terminal.type == HAUVIE_FSM_ACTION_FAILED);
    assert(runtime.fsm.state == HAUVIE_FSM_FAILED);
}

static void test_cancel_and_ignore_late_completion(void) {
    HauvieRuntime runtime;
    FakeProvider fake = {0};
    init_runtime(&runtime, &fake);
    assert(hauvie_runtime_start(&runtime));
    uint64_t alpha_id = fake.request_ids[0];

    hauvie_runtime_cancel(&runtime);
    assert(fake.cancelled == 1);
    assert(fake.cancelled_ids[0] == alpha_id);
    assert(fake.terminal_count == 1);
    assert(fake.terminal.type == HAUVIE_FSM_ACTION_CANCELLED);
    assert(runtime.fsm.state == HAUVIE_FSM_CANCELLED);

    hauvie_runtime_complete(&runtime, HAUVIE_FSM_ALPHA, alpha_id,
                            HAUVIE_FSM_RESULT_VALID);
    assert(fake.terminal_count == 1);
    assert(!hauvie_runtime_start(&runtime));
}

static void test_submit_rejection_uses_failover(void) {
    HauvieRuntime runtime;
    FakeProvider fake = {0};
    fake.rejected_provider = HAUVIE_FSM_ALPHA;
    init_runtime(&runtime, &fake);
    fake.rejected_provider = HAUVIE_FSM_ALPHA;

    assert(hauvie_runtime_start(&runtime));
    assert(fake.submitted == 2);
    assert(fake.providers[1] == HAUVIE_FSM_BETA);
    assert(runtime.fsm.state == HAUVIE_FSM_BETA_PENDING);

    fake.rejected_provider = HAUVIE_FSM_BETA;
    /* Beta was already accepted; its eventual error completes the FSM. */
    hauvie_runtime_complete(&runtime, HAUVIE_FSM_BETA, fake.request_ids[1],
                            HAUVIE_FSM_RESULT_TRANSPORT_ERROR);
    assert(fake.terminal_count == 1);
    assert(fake.terminal.type == HAUVIE_FSM_ACTION_FAILED);
}

int main(void) {
    test_init_enforces_payload_bounds();
    test_success_and_stale_completion();
    test_alpha_failover_beta_success_and_late_alpha();
    test_timeout_cancels_and_fails_over();
    test_cancel_and_ignore_late_completion();
    test_submit_rejection_uses_failover();
    puts("Async provider runtime tests passed.");
    return 0;
}
