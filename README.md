# Hauvie Helper Repair

A small C11 project that models a bounded Hauvie diagnostic pipeline and a reusable Alpha/Beta failover FSM. It remains a proof of concept: no general-purpose source-language parsing, production code generation, live model integration, or hardware control is implemented.

## Platform flow

![Hauvie helper pipeline architecture](docs/pipeline.svg)

```text
CLI input -> bounded token stream -> explicit AST state -> diagnostic core -> report
                                                                    |
                                                                    +-> router -> bounded C/Java starter template
                                                                    |
                                                                    +-> remediation policy -> repair queue policy
```

The pipeline separates syntax and domain failures. Invalid syntax is rejected before an AST is produced. Valid but critical patients are accepted by the parser and reported as diagnostic failures; the diagnostic result does not prevent the router from selecting a C or Java starter template.

## Token stream processing

`include/hauvie_token_stream.h` and `src/token_stream.c` provide a bounded, non-owning lexical stream over a NUL-terminated input string. The stream emits `WORD` and `EQUALS` tokens with source locations and explicit errors for invalid or oversized input.

Limits are explicit: source length ≤ 512 bytes, token length ≤ 64 bytes, and at most 32 emitted tokens. The `key=value` parser in `src/token.c` consumes tokens incrementally from this stream and verifies a caller-owned AST state.

## Router output

`include/hauvie_router.h` and `src/router.c` emit fixed C or Java starter templates. The original `hauvie_router_render(FILE *, ...)` API remains available for direct stream output.

The bounded API, `hauvie_router_render_buffer()`, is suited to embedded or caller-owned storage:

- A sizing call uses `buffer == NULL` and `capacity == 0` and returns the required bytes in `required`.
- `written` reports bytes stored, excluding the NUL terminator.
- Undersized nonzero buffers receive a truncated, NUL-terminated prefix and still return success.
- Call with at least `required + 1` bytes to obtain the complete template.
- Invalid arguments and unsupported targets return failure with both length outputs reset to zero.

This router is a rendering boundary, not a compiler or code generator. The template size and behavior are intentionally fixed.

## Diagnostic remediation policy

`include/hauvie_remediation.h` and `src/remediation.c` keep policy separate from diagnosis and execution:

| Pipeline result | Remediation |
| --- | --- |
| `PIPELINE_OK` | `HAUVIE_REMEDIATION_NONE` |
| `PIPELINE_CRITICAL_HAUVIES` | `HAUVIE_REMEDIATION_REVIEW` |
| `PIPELINE_INVALID_ARGUMENT` | `HAUVIE_REMEDIATION_UNSPECIFIED` |
| `PIPELINE_INFRASTRUCTURE_FAILURE` | `HAUVIE_REMEDIATION_UNSPECIFIED` |

An unspecified policy does not imply an action, including for unknown future values. The existing repair queue interprets critical patient diagnostics by creating a bounded `action=review` request; it does not execute remediation itself. Provider callbacks remain responsible for external work.

See [Diagnostic remediation and bounded router output](docs/remediation.md), [Pipeline architecture](docs/pipeline.md), and [Repair queue integration](docs/repair-queue.md).

## Reusable Alpha/Beta failover FSM

Public interface: `fsm/include/hauvie_fsm.h`; implementation: `fsm/src/hauvie_fsm.c`. It is an event-driven state machine only; it creates no threads, event loop, timers, or provider requests. The caller handles returned actions and supplies completion and timeout events. Stale provider/request IDs are ignored.

## Async provider runtime

`fsm/include/hauvie_runtime.h` and `fsm/src/hauvie_runtime.c` provide an event-loop-agnostic asynchronous adapter for the FSM. A caller supplies submit/cancel callbacks and delivers completion or timeout events to the runtime. Provider requests receive monotonically increasing IDs, Alpha failures fall back to Beta, and stale events are ignored.

The owner must keep request payloads alive and unchanged through the terminal callback. Payloads are capped at 512 bytes. Calls must be serialized on one event-loop thread; callbacks must be bounded and nonblocking and must not re-enter the runtime.

See [docs/runtime-api.md](docs/runtime-api.md) for payload validation, callback semantics, cancellation, timeout, and lifetime policies.

## Build and test

Requires a C11 compiler and Make:

```sh
make
make test
```

The focused router/remediation tests run with:

```sh
make test-router-remediation
./test-router-remediation
```

Tests cover core diagnostics, AST state, token boundaries and parser integration, FSM transitions, asynchronous runtime behavior, and repair-queue integration.

## Current limitations

No stream-driven code generation, C/Java source parsing, live model integration, or hardware control is implemented. Router output is fixed starter text, and remediation is an explicit policy lookup rather than an execution engine. The failover runtime remains the caller's responsibility, including its event loop and provider callbacks.
