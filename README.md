# Hauvie Helper Repair

A small C project that models a Hauvie diagnostic pipeline. It currently provides a bounded input DSL, a deterministic diagnostic stub, fixed C/Java starter-template output, and a reusable Alpha/Beta failover FSM. It is a scaffold for defining real code-extruder behavior—not a general-purpose source-code parser, compiler, or production diagnostic system.

## Current pipeline

![Hauvie helper pipeline architecture](docs/pipeline.svg)

```text
CLI input -> token.c -> explicit AST state -> diagnostic core -> PipelineReport -> router.c
                                  async adapter -> reusable Alpha/Beta FSM
```

The `fsm/` directory contains a provider-neutral, event-driven state machine. It does not create threads or call a model service; an application event loop dispatches its `SUBMIT` actions and later delivers completion, timeout, or cancellation events. This keeps the FSM reusable and lets the current PoC use deterministic mock completions.

## Failover FSM

Public interface: `fsm/include/hauvie_fsm.h`; implementation: `fsm/src/hauvie_fsm.c`.

```text
IDLE -> ALPHA_PENDING
          | valid parsed response -> VERIFIED -> core diagnostic
          | invalid / transport error / timeout -> BETA_PENDING
                                                  | valid -> VERIFIED
                                                  | invalid / error / timeout -> FAILED
pending -> CANCELLED
```

Each submission carries a provider and request ID. Late or mismatched responses are ignored, and cancellation makes any later completion stale. The adapter must parse/validate a provider response before reporting `HAUVIE_FSM_RESULT_VALID`. A syntactically valid but domain-critical AST is accepted as verified; severity belongs to the later diagnostic stage and must not trigger provider failover. Timeouts are driven by the caller's scheduler and delivered through `hauvie_fsm_timeout`.

The FSM is single-owner and not internally thread-safe. Integrations should deliver events serially on their event loop, or add synchronization at the adapter boundary. Request IDs correlate responses; they are not a substitute for provider authentication or transport security.

## Existing input and diagnostic behavior

- `src/main.c` accepts positional patient/severity arguments or `--source` input.
- `src/token.c` parses a limited, bounded whitespace-separated `key=value` DSL, not C, Java, or arbitrary source code.
- The AST is a caller-owned state object. It transitions from unverified to verified or rejected and is never represented by a null pointer for ordinary parse failure.
- `src/hauvie.c` validates patient data and returns a structured diagnostic. The three-cycle success loop remains a placeholder; no actual code extrusion or remediation occurs.
- `src/router.c` emits fixed C or Java starter templates. It does not compile or execute generated code.
- Severity thresholds and metrics are illustrative project rules, not medically or operationally validated facts.

## Build and tests

Requires a C11 compiler and Make.

```sh
make
make test
```

`make test` includes direct FSM tests for Alpha success, Alpha-to-Beta failover, both providers failing, timeout, cancellation, stale completions, and valid responses that should continue to domain diagnostics.
