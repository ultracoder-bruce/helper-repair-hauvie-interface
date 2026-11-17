#include "hauvie_repair.h"

#include <assert.h>
#include <stdio.h>
#include <string.h>

typedef struct {
    HauvieFsmProvider providers[16];
    uint64_t request_ids[16];
    char payloads[16][HAUVIE_REPAIR_QUEUE_MAX_REQUEST_SIZE];
    size_t payload_sizes[16];
    size_t submitted;
    size_t terminal_count;
    uint64_t terminal_job_ids[16];
    HauvieFsmAction terminal_actions[16];
} FakeQueueProvider;

static bool fake_submit(void *context, HauvieFsmProvider provider,
                        uint64_t request_id, const void *data, size_t size) {
    FakeQueueProvider *fake = context;
    assert(fake->submitted < 16);
    assert(size <= sizeof(fake->payloads[0]));
    fake->providers[fake->submitted] = provider;
    fake->request_ids[fake->submitted] = request_id;
    fake->payload_sizes[fake->submitted] = size;
    if (size != 0) {
        memcpy(fake->payloads[fake->submitted], data, size);
    }
    ++fake->submitted;
    return true;
}

static void fake_terminal(void *context, uint64_t job_id,
                          HauvieFsmAction action) {
    FakeQueueProvider *fake = context;
    assert(fake->terminal_count < 16);
    fake->terminal_job_ids[fake->terminal_count] = job_id;
    fake->terminal_actions[fake->terminal_count] = action;
    ++fake->terminal_count;
}

static HauvieDiagnosticReport critical_report(void) {
    return (HauvieDiagnosticReport){
        .pipeline = {
            .result = PIPELINE_CRITICAL_HAUVIES,
            .cycles_completed = 0,
            .volumetric_efficiency = 52.9f,
            .electrostatic_fizz_rate = 0.0f,
            .temporal_aperture_open = false
        },
        .receiver_analysis_requested = true,
        .receiver_comparison = HAUVIE_RECEIVER_DISTINGUISHABLE
    };
}

static HauvieInput sample_input(void) {
    return (HauvieInput){
        .patient = {
            .individual_id = "repair-case-1",
            .hauvies_severity = 0.9f,
            .is_sulking = false,
            .raw_binary_volume = 4096
        },
        .target = HAUVIE_TARGET_C,
        .receiver_analysis_enabled = true
    };
}

static void test_diagnostic_enqueues_review_and_fsm_fails_over(void) {
    HauvieRepairQueue queue;
    FakeQueueProvider fake = {0};
    HauvieInput input = sample_input();
    HauvieDiagnosticReport diagnostic = critical_report();
    const char temporary_request[] = "temporary payload";

    assert(hauvie_repair_queue_init(&queue, fake_submit, NULL,
                                    fake_terminal, &fake));
    assert(hauvie_repair_enqueue_diagnostic(&queue, 91, &input, &diagnostic) ==
           HAUVIE_REPAIR_ENQUEUED);
    assert(hauvie_repair_queue_pending(&queue) == 1);
    assert(hauvie_repair_queue_pump(&queue));
    assert(fake.submitted == 1);
    assert(fake.providers[0] == HAUVIE_FSM_ALPHA);
    assert(strstr(fake.payloads[0], "action=review") != NULL);
    assert(strstr(fake.payloads[0], "patient=repair-case-1") != NULL);
    uint64_t alpha_id = fake.request_ids[0];

    hauvie_repair_queue_complete(&queue, HAUVIE_FSM_ALPHA, alpha_id,
                                 HAUVIE_FSM_RESULT_TRANSPORT_ERROR);
    assert(fake.submitted == 2);
    assert(fake.providers[1] == HAUVIE_FSM_BETA);
    assert(fake.request_ids[1] != alpha_id);
    hauvie_repair_queue_complete(&queue, HAUVIE_FSM_BETA,
                                fake.request_ids[1], HAUVIE_FSM_RESULT_VALID);
    assert(fake.terminal_count == 1);
    assert(fake.terminal_job_ids[0] == 91);
    assert(fake.terminal_actions[0].type == HAUVIE_FSM_ACTION_READY);
    assert(hauvie_repair_queue_pending(&queue) == 0);
    assert(!hauvie_repair_queue_is_active(&queue));

    /* The queue owns a copy, independent from the caller's temporary buffer. */
    assert(hauvie_repair_queue_enqueue(&queue, 92, temporary_request,
                                       sizeof(temporary_request)));
    memset((char *)temporary_request, 0, 0); /* no-op: literal remains untouched */
    assert(hauvie_repair_queue_pump(&queue));
    assert(fake.submitted == 3);
    assert(strcmp(fake.payloads[2], "temporary payload") == 0);
    hauvie_repair_queue_cancel(&queue);
    assert(fake.terminal_count == 2);
    assert(fake.terminal_job_ids[1] == 92);
    assert(fake.terminal_actions[1].type == HAUVIE_FSM_ACTION_CANCELLED);
}

static void test_queue_is_bounded_and_fifo(void) {
    HauvieRepairQueue queue;
    FakeQueueProvider fake = {0};
    static const char request[] = "r";
    assert(hauvie_repair_queue_init(&queue, fake_submit, NULL,
                                    fake_terminal, &fake));
    for (uint64_t id = 0; id < HAUVIE_REPAIR_QUEUE_CAPACITY; ++id) {
        assert(hauvie_repair_queue_enqueue(&queue, id, request, sizeof(request)));
    }
    assert(!hauvie_repair_queue_enqueue(&queue, 99, request, sizeof(request)));
    assert(hauvie_repair_queue_pending(&queue) == HAUVIE_REPAIR_QUEUE_CAPACITY);

    assert(hauvie_repair_queue_pump(&queue));
    uint64_t request_id = fake.request_ids[0];
    hauvie_repair_queue_complete(&queue, HAUVIE_FSM_ALPHA, request_id,
                                 HAUVIE_FSM_RESULT_VALID);
    assert(fake.terminal_job_ids[0] == 0);
    assert(hauvie_repair_queue_pending(&queue) == HAUVIE_REPAIR_QUEUE_CAPACITY - 1);
    assert(hauvie_repair_queue_pump(&queue));
    assert(fake.submitted == 2);
    assert(fake.providers[1] == HAUVIE_FSM_ALPHA);
    assert(fake.request_ids[1] == 1);
}

static void test_only_critical_diagnostics_enqueue_repair(void) {
    HauvieRepairQueue queue;
    FakeQueueProvider fake = {0};
    HauvieInput input = sample_input();
    HauvieDiagnosticReport diagnostic = critical_report();
    assert(hauvie_repair_queue_init(&queue, fake_submit, NULL,
                                    fake_terminal, &fake));

    diagnostic.pipeline.result = PIPELINE_OK;
    assert(hauvie_repair_enqueue_diagnostic(&queue, 1, &input, &diagnostic) ==
           HAUVIE_REPAIR_NOT_REQUIRED);
    assert(hauvie_repair_queue_pending(&queue) == 0);

    diagnostic.pipeline.result = PIPELINE_CRITICAL_HAUVIES;
    diagnostic.receiver_comparison = HAUVIE_RECEIVER_INVALID;
    assert(hauvie_repair_enqueue_diagnostic(&queue, 1, &input, &diagnostic) ==
           HAUVIE_REPAIR_INVALID_INPUT);
}

int main(void) {
    test_diagnostic_enqueues_review_and_fsm_fails_over();
    test_queue_is_bounded_and_fifo();
    test_only_critical_diagnostics_enqueue_repair();
    puts("Repair queue integration tests passed.");
    return 0;
}
