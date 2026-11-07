#include "hauvie_fsm.h"

#include <limits.h>

static HauvieFsmAction action(HauvieFsmActionType type,
                              HauvieFsmProvider provider,
                              uint64_t request_id) {
    HauvieFsmAction result = {type, provider, request_id};
    return result;
}

static bool pending_state(HauvieFsmState state) {
    return state == HAUVIE_FSM_ALPHA_PENDING ||
           state == HAUVIE_FSM_BETA_PENDING;
}

static HauvieFsmAction fail_active(HauvieFsm *fsm) {
    if (fsm->state == HAUVIE_FSM_ALPHA_PENDING) {
        if (fsm->next_request_id == UINT64_MAX) {
            fsm->state = HAUVIE_FSM_FAILED;
            return action(HAUVIE_FSM_ACTION_FAILED, HAUVIE_FSM_BETA, 0);
        }
        fsm->active_provider = HAUVIE_FSM_BETA;
        fsm->active_request_id = fsm->next_request_id++;
        fsm->state = HAUVIE_FSM_BETA_PENDING;
        return action(HAUVIE_FSM_ACTION_SUBMIT, HAUVIE_FSM_BETA,
                      fsm->active_request_id);
    }

    fsm->state = HAUVIE_FSM_FAILED;
    fsm->active_request_id = 0;
    return action(HAUVIE_FSM_ACTION_FAILED, fsm->active_provider, 0);
}

void hauvie_fsm_init(HauvieFsm *fsm) {
    if (fsm == NULL) {
        return;
    }
    fsm->state = HAUVIE_FSM_IDLE;
    fsm->active_request_id = 0;
    fsm->next_request_id = 1;
    fsm->active_provider = HAUVIE_FSM_ALPHA;
}

HauvieFsmAction hauvie_fsm_start(HauvieFsm *fsm) {
    if (fsm == NULL || fsm->state != HAUVIE_FSM_IDLE ||
        fsm->next_request_id == UINT64_MAX) {
        return action(HAUVIE_FSM_ACTION_NONE, HAUVIE_FSM_ALPHA, 0);
    }

    fsm->active_provider = HAUVIE_FSM_ALPHA;
    fsm->active_request_id = fsm->next_request_id++;
    fsm->state = HAUVIE_FSM_ALPHA_PENDING;
    return action(HAUVIE_FSM_ACTION_SUBMIT, HAUVIE_FSM_ALPHA,
                  fsm->active_request_id);
}

HauvieFsmAction hauvie_fsm_complete(
    HauvieFsm *fsm,
    HauvieFsmProvider provider,
    uint64_t request_id,
    HauvieFsmResult result
) {
    if (fsm == NULL || !pending_state(fsm->state) ||
        provider != fsm->active_provider || request_id == 0 ||
        request_id != fsm->active_request_id) {
        return action(HAUVIE_FSM_ACTION_NONE, provider, request_id);
    }

    if (result == HAUVIE_FSM_RESULT_VALID) {
        fsm->state = HAUVIE_FSM_VERIFIED;
        fsm->active_request_id = 0;
        return action(HAUVIE_FSM_ACTION_READY, provider, request_id);
    }

    return fail_active(fsm);
}

HauvieFsmAction hauvie_fsm_timeout(
    HauvieFsm *fsm,
    HauvieFsmProvider provider,
    uint64_t request_id
) {
    if (fsm == NULL || !pending_state(fsm->state) ||
        provider != fsm->active_provider || request_id == 0 ||
        request_id != fsm->active_request_id) {
        return action(HAUVIE_FSM_ACTION_NONE, provider, request_id);
    }
    return fail_active(fsm);
}

HauvieFsmAction hauvie_fsm_cancel(HauvieFsm *fsm) {
    if (fsm == NULL || !pending_state(fsm->state)) {
        return action(HAUVIE_FSM_ACTION_NONE, HAUVIE_FSM_ALPHA, 0);
    }
    fsm->state = HAUVIE_FSM_CANCELLED;
    fsm->active_request_id = 0;
    return action(HAUVIE_FSM_ACTION_CANCELLED, fsm->active_provider, 0);
}
