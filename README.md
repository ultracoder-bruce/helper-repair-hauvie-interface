# Hauvie Helper Repair

A small C11 project that models a bounded Hauvie diagnostic pipeline and a reusable Alpha/Beta failover FSM. It remains a proof of concept: no general-purpose source-language parsing, production code generation, model-provider transport, or hardware integration is implemented.

## Token stream processing (first draft)

`include/hauvie_token_stream.h` and `src/token_stream.c` provide a bounded, non-owning lexical stream over a NUL-terminated input string. The stream emits `WORD` and `EQUALS` tokens, with source slices and byte offsets; callers receive `END` on clean exhaustion or `ERROR` for invalid characters, overlong input/tokens, or token-count exhaustion. It allocates no heap memory, and tokens borrow the source buffer, so the source must remain alive and unchanged while tokens are used.

Limits are explicit: source length ≤ 512 bytes, token length ≤ 64 bytes, and at most 32 emitted tokens. The `key=value` parser in `src/token.c` now consumes tokens incrementally from this stream rather than building an intermediate token array. This is a first-draft tokenizer for the project's small DSL—not a C/Java lexer and not arbitrary code parsing. Quotes, escapes, comments, and Unicode identifiers are not supported.

Example token sequence for `patient=alpha severity=0.3`:

```text
WORD("patient") EQUALS WORD("alpha") WORD("severity") EQUALS WORD("0.3") END
```

## Pipeline overview

![Hauvie helper pipeline architecture](docs/pipeline.svg)

```text
CLI input -> bounded token stream -> explicit AST state -> diagnostic core -> report -> C/Java starter template
```

The AST state is caller-owned and explicit: it is initialized as `UNVERIFIED`, then becomes `VERIFIED` or `REJECTED` with an error. Syntax validity is distinct from domain severity; a valid critical input remains a valid AST and proceeds to diagnostic handling.

## Input and diagnostic behavior

The CLI accepts:

```sh
./hauvie-helper [patient-id [severity]]
./hauvie-helper --source 'patient=Tech-Agent-404 severity=0.2 sulking=false target=c'
```

The source DSL supports unique, whitespace-separated `patient`, `severity`, `sulking`, and `target` fields. Defaults are patient `Tech-Agent-404`, severity `0.2`, `false`, and `c`. Severity must be finite and in `[0,1]`; values ≥ `0.75` or `sulking=true` produce `PIPELINE_CRITICAL_HAUVIES`. These metrics are illustrative rules, not validated medical or hardware facts.

`src/hauvie.c` currently validates and reports status; its three-cycle success loop is a placeholder, not actual extrusion. `src/router.c` emits fixed C/Java starter templates and does not compile or execute generated output.

## Reusable Alpha/Beta failover FSM

Public interface: `fsm/include/hauvie_fsm.h`; implementation: `fsm/src/hauvie_fsm.c`. It is an event-driven state machine only; it creates no threads, event loop, timers, or provider requests. The integrating application dispatches `SUBMIT` actions and later delivers completion, timeout, and cancellation events. Alpha transport/validation failure triggers Beta; valid parsed input advances to `VERIFIED`. Domain-critical severity is handled by the diagnostic core and does not trigger failover. Mismatched and late request IDs are ignored. Calls must be serialized by the caller because the FSM is not thread-safe.

## Build and tests

Requires a C11 compiler and Make:

```sh
make
make test
```

Tests cover core diagnostics, AST state, token stream boundaries and parser integration, plus FSM transitions.

## Current limitations

No stream-driven code generation, bounded output-buffer writer, C/Java source parsing, actual extrusion/remediation rules, live model integration, async runtime, or hardware control has been implemented. Those require separate specifications and tests before implementation.
