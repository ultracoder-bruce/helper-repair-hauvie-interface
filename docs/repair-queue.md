# Repair request queue and FSM integration

The repair queue is a fixed-capacity FIFO (eight entries) layered around the
existing Alpha/Beta asynchronous FSM runtime. Request payloads are copied into
queue-owned storage, capped at the runtime's 512-byte limit, and submitted one
at a time. Call `hauvie_repair_queue_pump` to start the head; deliver provider
completion and timeout events with the active provider/request ID. The FSM
performs Alpha-to-Beta failover, ignores stale events, and the queue reports a
terminal action with the associated job ID. Pump again to dispatch the next
queued request.

`hauvie_repair_enqueue_diagnostic` bridges diagnostics into the queue: it
creates a bounded `action=review` request only for a critical patient result.
Non-critical results do not enqueue work. The queue is a transport/workflow
adapter, not an automatic remediation engine; provider callbacks own any
external action and should be nonblocking, serialized, and non-reentrant as
specified in `docs/runtime-api.md`.

Build and run the focused integration test with `make test-repair-queue`.
