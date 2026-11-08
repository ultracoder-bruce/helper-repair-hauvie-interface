#include "hauvie_runtime.h"

static void dispatch_action(HauvieRuntime *runtime, HauvieFsmAction action);

static void dispatch_action(HauvieRuntime *runtime, HauvieFsmAction action) {
    switch (action.type) {
        case HAUVIE_FSM_ACTION_SUBMIT:
            if (!runtime->submit(runtime->context, action.provider,
                                 action.request_id, runtime->request_data,
                                 runtime->request_size)) {
                HauvieFsmAction next = hauvie_fsm_complete(
                    &runtime->fsm, action.provider, action.request_id,
                    HAUVIE_FSM_RESULT_TRANSPORT_ERROR);
                dispatch_action(runtime, next);
            }
            break;
        case HAUVIE_FSM_ACTION_READY:
        case HAUVIE_FSM_ACTION_FAILED:
        case HAUVIE_FSM_ACTION_CANCELLED:
            if (runtime->on_terminal != NULL) {
                runtime->on_terminal(runtime->context, action);
            }
            break;
        case HAUVIE_FSM_ACTION_NONE:
        default:
            break;
    }
}

bool hauvie_runtime_init(
    HauvieRuntime *runtime,
    HauvieRuntimeSubmitFn submit,
    HauvieRuntimeCancelFn cancel,
    HauvieRuntimeTerminalFn on_terminal,
    void *context,
    const void *request_data,
    size_t request_size
) {
    if (runtime == NULL || submit == NULL ||
        (request_data == NULL && request_size != 0)) {
        return false;
    }

    hauvie_fsm_init(&runtime->fsm);
    runtime->submit = submit;
    runtime->cancel = cancel;
    runtime->on_terminal = on_terminal;
    runtime->context = context;
    runtime->request_data = request_data;
    runtime->request_size = request_size;
    runtime->initialized = true;
    return true;
}

bool hauvie_runtime_start(HauvieRuntime *runtime) {
    if (runtime == NULL || !runtime->initialized) {
        return false;
    }

    HauvieFsmAction action = hauvie_fsm_start(&runtime->fsm);
    if (action.type != HAUVIE_FSM_ACTION_SUBMIT) {
        return false;
    }
    dispatch_action(runtime, action);
    return true;
}

void hauvie_runtime_complete(
    HauvieRuntime *runtime,
    HauvieFsmProvider provider,
    uint64_t request_id,
    HauvieFsmResult result
) {
    if (runtime == NULL || !runtime->initialized) {
        return;
    }

    HauvieFsmAction action = hauvie_fsm_complete(
        &runtime->fsm, provider, request_id, result);
    dispatch_action(runtime, action);
}

void hauvie_runtime_timeout(
    HauvieRuntime *runtime,
    HauvieFsmProvider provider,
    uint64_t request_id
) {
    if (runtime == NULL || !runtime->initialized) {
        return;
    }

    HauvieFsmAction action = hauvie_fsm_timeout(
        &runtime->fsm, provider, request_id);
    if (action.type == HAUVIE_FSM_ACTION_NONE) {
        return;
    }

    if (runtime->cancel != NULL) {
        runtime->cancel(runtime->context, provider, request_id);
    }
    dispatch_action(runtime, action);
}

void hauvie_runtime_cancel(HauvieRuntime *runtime) {
    if (runtime == NULL || !runtime->initialized) {
        return;
    }

    HauvieFsmProvider provider = runtime->fsm.active_provider;
    uint64_t request_id = runtime->fsm.active_request_id;
    HauvieFsmAction action = hauvie_fsm_cancel(&runtime->fsm);
    if (action.type != HAUVIE_FSM_ACTION_CANCELLED) {
        return;
    }

    if (runtime->cancel != NULL) {
        runtime->cancel(runtime->context, provider, request_id);
    }
    dispatch_action(runtime, action);
}
