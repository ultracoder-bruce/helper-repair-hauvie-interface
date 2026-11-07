#include "hauvie_fsm.h"

#include <assert.h>
#include <stdio.h>

static void test_alpha_success(void) {
    HauvieFsm fsm;
    hauvie_fsm_init(&fsm);
    HauvieFsmAction submit = hauvie_fsm_start(&fsm);
    assert(submit.type == HAUVIE_FSM_ACTION_SUBMIT);
    assert(submit.provider == HAUVIE_FSM_ALPHA);
    HauvieFsmAction ready = hauvie_fsm_complete(
        &fsm, HAUVIE_FSM_ALPHA, submit.request_id, HAUVIE_FSM_RESULT_VALID);
    assert(ready.type == HAUVIE_FSM_ACTION_READY);
    assert(fsm.state == HAUVIE_FSM_VERIFIED);
}

static void test_alpha_failure_beta_success_and_late_alpha(void) {
    HauvieFsm fsm;
    hauvie_fsm_init(&fsm);
    HauvieFsmAction alpha = hauvie_fsm_start(&fsm);
    HauvieFsmAction beta = hauvie_fsm_complete(
        &fsm, HAUVIE_FSM_ALPHA, alpha.request_id,
        HAUVIE_FSM_RESULT_TRANSPORT_ERROR);
    assert(beta.type == HAUVIE_FSM_ACTION_SUBMIT);
    assert(beta.provider == HAUVIE_FSM_BETA);
    assert(fsm.state == HAUVIE_FSM_BETA_PENDING);

    HauvieFsmAction late = hauvie_fsm_complete(
        &fsm, HAUVIE_FSM_ALPHA, alpha.request_id, HAUVIE_FSM_RESULT_VALID);
    assert(late.type == HAUVIE_FSM_ACTION_NONE);
    assert(fsm.state == HAUVIE_FSM_BETA_PENDING);

    HauvieFsmAction ready = hauvie_fsm_complete(
        &fsm, HAUVIE_FSM_BETA, beta.request_id, HAUVIE_FSM_RESULT_VALID);
    assert(ready.type == HAUVIE_FSM_ACTION_READY);
    assert(fsm.state == HAUVIE_FSM_VERIFIED);
}

static void test_both_fail(void) {
    HauvieFsm fsm;
    hauvie_fsm_init(&fsm);
    HauvieFsmAction alpha = hauvie_fsm_start(&fsm);
    HauvieFsmAction beta = hauvie_fsm_complete(
        &fsm, HAUVIE_FSM_ALPHA, alpha.request_id, HAUVIE_FSM_RESULT_INVALID);
    assert(beta.type == HAUVIE_FSM_ACTION_SUBMIT);
    HauvieFsmAction failed = hauvie_fsm_complete(
        &fsm, HAUVIE_FSM_BETA, beta.request_id, HAUVIE_FSM_RESULT_TRANSPORT_ERROR);
    assert(failed.type == HAUVIE_FSM_ACTION_FAILED);
    assert(fsm.state == HAUVIE_FSM_FAILED);
}

static void test_timeout_and_cancel(void) {
    HauvieFsm fsm;
    hauvie_fsm_init(&fsm);
    HauvieFsmAction alpha = hauvie_fsm_start(&fsm);
    HauvieFsmAction beta = hauvie_fsm_timeout(
        &fsm, HAUVIE_FSM_ALPHA, alpha.request_id);
    assert(beta.type == HAUVIE_FSM_ACTION_SUBMIT);
    HauvieFsmAction cancelled = hauvie_fsm_cancel(&fsm);
    assert(cancelled.type == HAUVIE_FSM_ACTION_CANCELLED);
    assert(fsm.state == HAUVIE_FSM_CANCELLED);
    assert(hauv ie_fsm_complete(&fsm, HAUVIE_FSM_BETA, beta.request_id,
                                HAUVIE_FSM_RESULT_VALID).type == HAUVIE_FSM_ACTION_NONE);
}

static void test_valid_but_domain_critical_is_not_failover(void) {
    HauvieFsm fsm;
    hauvie_fsm_init(&fsm);
    HauvieFsmAction alpha = hauvie_fsm_start(&fsm);
    HauvieFsmAction ready = hauvie_fsm_complete(
        &fsm, HAUVIE_FSM_ALPHA, alpha.request_id, HAUVIE_FSM_RESULT_VALID);
    assert(ready.type == HAUVIE_FSM_ACTION_READY);
    assert(fsm.state == HAUVIE_FSM_VERIFIED);
}

int main(void) {
    test_alpha_success();
    test_alpha_failure_beta_success_and_late_alpha();
    test_both_fail();
    test_timeout_and_cancel();
    test_valid_but_domain_critical_is_not_failover();
    puts("Reusable Alpha/Beta FSM tests passed.");
    return 0;
}
