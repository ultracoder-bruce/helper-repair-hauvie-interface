#ifndef HAUVIE_FSM_H
#define HAUVIE_FSM_H

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    HAUVIE_FSM_IDLE = 0,
    HAUVIE_FSM_ALPHA_PENDING,
    HAUVIE_FSM_BETA_PENDING,
    HAUVIE_FSM_VERIFIED,
    HAUVIE_FSM_FAILED,
    HAUVIE_FSM_CANCELLED
} HauvieFsmState;

typedef enum {
    HAUVIE_FSM_ALPHA = 0,
    HAUVIE_FSM_BETA
} HauvieFsmProvider;

typedef enum {
    HAUVIE_FSM_ACTION_NONE = 0,
    HAUVIE_FSM_ACTION_SUBMIT,
    HAUVIE_FSM_ACTION_READY,
    HAUVIE_FSM_ACTION_FAILED,
    HAUVIE_FSM_ACTION_CANCELLED
} HauvieFsmActionType;

typedef enum {
    HAUVIE_FSM_RESULT_VALID = 0,
    HAUVIE_FSM_RESULT_TRANSPORT_ERROR,
    HAUVIE_FSM_RESULT_INVALID
} HauvieFsmResult;

typedef struct {
    HauvieFsmState state;
    uint64_t active_request_id;
    uint64_t next_request_id;
    HauvieFsmProvider active_provider;
} HauvieFsm;

typedef struct {
    HauvieFsmActionType type;
    HauvieFsmProvider provider;
    uint64_t request_id;
} HauvieFsmAction;

/* Initialize a reusable, caller-owned failover state machine. */
void hauvie_fsm_init(HauvieFsm *fsm);

/* Begin Alpha. The returned SUBMIT action is dispatched by the caller's event loop. */
HauvieFsmAction hauvie_fsm_start(HauvieFsm *fsm);

/*
 * Deliver a completed asynchronous request. VALID means the response has
 * already been parsed and verified by the caller. A valid but domain-critical
 * AST is still VALID here and proceeds to core diagnostics without failover.
 * Stale provider/request IDs are ignored.
 */
HauvieFsmAction hauvie_fsm_complete(
    HauvieFsm *fsm,
    HauvieFsmProvider provider,
    uint64_t request_id,
    HauvieFsmResult result
);

/* Treat the currently active request as timed out and trigger normal failover. */
HauvieFsmAction hauvie_fsm_timeout(
    HauvieFsm *fsm,
    HauvieFsmProvider provider,
    uint64_t request_id
);

/* Cancel pending work. Later completions are ignored. */
HauvieFsmAction hauvie_fsm_cancel(HauvieFsm *fsm);

#ifdef __cplusplus
}
#endif

#endif
