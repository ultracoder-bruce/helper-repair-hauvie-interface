# Async provider runtime API contract

This document defines the caller and provider obligations for `fsm/include/hauvie_runtime.h`. These policies are part of the API contract; the runtime enforces payload pointer/length consistency and the maximum request size, while callers and provider adapters must honor lifecycle, scheduling, and callback rules.

## Request payload policy

- `request_size` must be at most `HAUVIE_RUNTIME_MAX_REQUEST_SIZE` (512 bytes). `hauvie_runtime_init()` rejects larger requests.
- A `NULL` payload is valid only when `request_size == 0`. A non-`NULL` payload must point to at least `request_size` readable bytes; C cannot verify the actual allocation size.
- The runtime borrows the payload. The owner must keep it alive and unchanged from successful initialization until `on_terminal` runs, or until the runtime is otherwise abandoned without a terminal result.
- Provider adapters must not retain the payload after `on_terminal`; if data must outlive the request, copy it into provider-owned storage before returning from `submit`.
- Zero-length payloads are valid. No NUL terminator is implied; the provider receives an explicit byte count.

## Scheduling and callback policy

- One `HauvieRuntime` instance is single-threaded and not thread-safe. The owner must serialize calls to `start`, `complete`, `timeout`, and `cancel`, as well as callbacks, on one event-loop thread or under equivalent external serialization.
- `submit` must enqueue the provider operation and return promptly; it must not block on network, disk, model inference, or other unbounded work. It returns `true` only if the request was accepted for asynchronous processing. Returning `false` is an immediate transport failure and triggers normal failover.
- `cancel` is best-effort and must return promptly. It may be `NULL` if the provider cannot cancel. Cancellation does not guarantee that the provider stops work; late completions must still be delivered or discarded safely, and the runtime ignores stale request IDs.
- `on_terminal` is optional and is called at most once for the single FSM run, when the FSM reaches verified, failed, or cancelled state.
- Callbacks must not call runtime APIs reentrantly. Schedule completion/timeout delivery as a later event-loop task. This also applies to a provider that completes immediately: queue its completion rather than delivering it inside `submit`.
- Callback implementations should not retain pointers to `HauvieRuntime` fields or mutate the runtime directly. The runtime owns FSM transitions; callbacks report outcomes through the public functions.

## State and event policy

- `hauvie_runtime_start()` starts one run, submitting to Alpha. It returns `false` if the runtime is uninitialized or the FSM is not idle; it does not restart a terminal run.
- Completion and timeout events must echo both the provider and request ID provided to `submit`. An event with a stale/mismatched provider or ID has no effect.
- Any non-`VALID` completion and a matching timeout fail Alpha over to Beta. The same outcomes from Beta produce a terminal failure. A `VALID` completion terminates successfully; diagnostic/domain policy is outside this runtime.
- A matching timeout transitions the FSM before invoking `cancel` for that timed-out provider request, then dispatches any fallback submission. A manual cancel transitions to cancelled, best-effort cancels the active request, and then notifies `on_terminal`.
- If a provider reports that submission was rejected (`false`), it must not later report a completion for that rejected request. Such a late event is stale after failover and ignored.

## Minimal integration sketch

```c
static bool submit(void *ctx, HauvieFsmProvider provider, uint64_t id,
                   const void *bytes, size_t length) {
    /* Queue provider work and preserve/copy bytes as required. */
    return queue_request(ctx, provider, id, bytes, length);
}

HauvieRuntime runtime;
if (!hauvie_runtime_init(&runtime, submit, cancel, on_terminal, app,
                         request_bytes, request_length)) {
    /* Reject invalid payload size or arguments. */
}
if (!hauvie_runtime_start(&runtime)) {
    /* Runtime was not idle or was not initialized. */
}

/* Later, on the serialized event loop: */
hauvie_runtime_complete(&runtime, provider, request_id,
                        HAUVIE_FSM_RESULT_VALID);
```
