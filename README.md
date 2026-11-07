# Hauvie Helper Repair

A small C11 project that models a bounded Hauvie diagnostic pipeline and a reusable Alpha/Beta failover state machine. It is an architectural proof of concept: it does not connect to model services, parse general-purpose programming languages, generate production code, or implement real code-extruder remediation.

## Feature set at a glance

- A bounded `key=value` input DSL for patient diagnostics and output target selection.
- An explicit caller-owned AST/result-state object: unverified, verified, or rejected; ordinary parse failures do not use a null AST pointer.
- A deterministic diagnostic core with structured results and an illustrative critical-severity rule.
- Fixed C and Java starter-template output.
- A provider-neutral, event-driven Alpha → Beta failover FSM in the `fsm/` directory.
- Request-ID correlation, timeout/failure fallback, cancellation, and stale-response rejection in the FSM.

## Pipeline overview

![Hauvie helper pipeline architecture](docs/pipeline.svg)

The synchronous CLI path is:

```text
CLI input -> token.c -> AST state -> diagnostic core -> PipelineReport -> router.c
```

The failover FSM is a reusable control-flow component for an application adapter that obtains and validates provider responses:

```text
start -> Alpha pending -> verified response -> core diagnostic
                         \-> failure/timeout -> Beta pending -> verified response -> core diagnostic
                                                           \-> failure/timeout -> failed
```

It is not currently wired into the CLI path and does not create threads, an event loop, timers, network requests, or provider calls. An integrating application owns those mechanisms and feeds completion, timeout, and cancellation events to the FSM.

## Input and parsing

The CLI accepts positional diagnostic arguments or a `--source` DSL string:

```sh
./hauvie-helper [patient-id [severity]]
./hauvie-helper --source 'patient=Tech-Agent-404 severity=0.2 sulking=false target=c'
./hauvie-helper --source 'patient=Tech-Agent-404 severity=0.9 sulking=true target=java'
```

The DSL is a sequence of whitespace-separated `key=value` fields. Supported fields are:

| Field | Accepted values | Default |
| --- | --- | --- |
| `patient` | 1–31 characters from letters, digits, `_`, `-`, `.` | `Tech-Agent-404` |
| `severity` | finite number from `0.0` to `1.0` | `0.2` |
| `sulking` | `true` or `false` | `false` |
| `target` | `c` or `java` | `c` |

Fields can be supplied in any order. Duplicate or unknown fields, malformed syntax, unsupported characters, oversized input, and out-of-range values are rejected. This DSL parser is not a C, Java, or general source-code parser.

### AST state contract

`HauvieAst` in `include/hauvie_token.h` is a fixed-size, caller-owned parse-result object, not a heap-allocated general-language syntax tree. `hauvie_parse_ast` initializes a non-NULL output object to `HAUVIE_AST_UNVERIFIED`, then records either:

- `HAUVIE_AST_VERIFIED` with `HAUVIE_PARSE_ERROR_NONE` when syntax and input validation succeed; or
- `HAUVIE_AST_REJECTED` with an error code when the input is invalid.

Only verified input should be passed to the diagnostic core. A syntactically valid but high-severity input remains verified; domain severity is evaluated later and does not invalidate the AST. The older `hauvie_parse_input` helper is also available for callers that only need a `HauvieInput` on success.

## Diagnostic core and routing

- `src/hauvie.c` exposes `hauvie_helper_run(context, patient, report)` via `include/hauvie.h`. It validates the patient, initializes the context, leaves patient data unchanged, and returns a `PipelineResult` with optional report metrics.
- Severity at or above `HAUVIES_CRITICAL_THRESHOLD` (`0.75`) or `is_sulking=true` produces `PIPELINE_CRITICAL_HAUVIES`. This is an illustrative project rule, not a medically or operationally validated threshold.
- The three-cycle success loop is currently a placeholder. No actual code extrusion, token transformation, remediation, or memory-buffer pipeline is implemented.
- `src/router.c` emits a fixed C or Java starter template. It does not derive a program from the input, compile it, or execute it. A critical diagnostic can still be represented in a routed report; the CLI returns a failing process status for critical results.

## Reusable Alpha/Beta failover FSM

Public interface: `fsm/include/hauvie_fsm.h`; implementation: `fsm/src/hauvie_fsm.c`.

### States and transitions

- `IDLE`: ready for one run; `hauvie_fsm_start` returns an Alpha `SUBMIT` action.
- `ALPHA_PENDING`: awaiting the Alpha adapter's event.
- `BETA_PENDING`: Alpha failed, timed out, or returned an unusable response; the FSM asks the caller to submit Beta.
- `VERIFIED`: a provider response has been accepted; the caller can continue to core diagnostics.
- `FAILED`: both provider attempts failed or timed out.
- `CANCELLED`: pending work was cancelled.

For both providers, a valid response transitions to `VERIFIED`. An invalid response, transport error, or timeout from Alpha transitions to `BETA_PENDING`; the same outcome from Beta transitions to `FAILED`. Cancellation is supported while a request is pending. The FSM itself does not impose a wall-clock timeout: the caller's scheduler detects it and calls `hauvie_fsm_timeout` with the matching provider and request ID.

### Adapter responsibilities and safety rules

1. Initialize a `HauvieFsm` and call `hauvie_fsm_start`.
2. Dispatch the returned `SUBMIT` action to the named provider, retaining its request ID.
3. When a response arrives, parse and validate it into the application's explicit AST state. Report `HAUVIE_FSM_RESULT_VALID` only after that succeeds; otherwise report `HAUVIE_FSM_RESULT_INVALID`. Transport failures use `HAUVIE_FSM_RESULT_TRANSPORT_ERROR`.
4. On `READY`, pass only the verified input to the core diagnostic. A valid but domain-critical AST is still a valid provider response and must not trigger Beta fallback.
5. On a timeout, call `hauvie_fsm_timeout` with the active provider and request ID, then dispatch any returned Beta `SUBMIT` action.
6. Dispatch or observe cancellation as appropriate. Completion events whose provider/request ID no longer match the active request are ignored, including late Alpha results after Beta has started or late results after cancellation.

The FSM is single-owner and not internally thread-safe. Serialize its calls on one event loop, or add synchronization at the adapter boundary. Request IDs only correlate events within one FSM instance; they are not authentication, transport-security, or cross-process identifiers. The current mock/test layer exercises the transition logic; no live Alpha/Beta integration exists.

## Build and tests

Requires a C11 compiler and Make.

```sh
make
make test
```

The Makefile builds the CLI, core tests, and FSM tests. The FSM test cases are intended to cover Alpha success, Alpha-to-Beta fallback, both providers failing, timeout, cancellation, stale completions, and acceptance of a valid response independently of domain severity. Test execution has not yet been confirmed for the current repository revision.

## Project limits / next work

The domain-specific extrusion rules, actual token stream processing, bounded output-buffer implementation, async adapter/event loop, real provider transport, and production code generation remain future work. Keep model/provider output untrusted: validate it into the project’s explicit input representation before allowing it to reach the diagnostic core or output router.
