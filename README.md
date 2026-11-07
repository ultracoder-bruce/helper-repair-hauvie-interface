# Hauvie Helper Repair

A small C project that models a Hauvie diagnostic pipeline. It currently provides a bounded input DSL, a deterministic diagnostic stub, and fixed C/Java starter-template output. It is a scaffold for defining the real code-extruder behavior—not a general-purpose source-code parser, compiler, or production diagnostic system.

## Current pipeline

![Hauvie helper pipeline architecture](docs/pipeline.svg)

```text
CLI input -> token.c -> explicit AST state -> hauvie.c -> PipelineReport -> router.c -> C/Java starter template
```

- `src/main.c` is the CLI entry point. It accepts either positional patient/severity arguments or `--source` input.
- `src/token.c` parses a limited sequence of whitespace-separated `key=value` fields. This is a small configuration DSL, not a lexer/parser for C, Java, or arbitrary submitted code. Parsing uses an explicit result object: valid input becomes `HAUVIE_AST_VERIFIED`; malformed input becomes `HAUVIE_AST_REJECTED` with an error code. It does not represent ordinary parse failure as a null AST pointer.
- `src/hauvie.c` validates patient data and returns a structured diagnostic result. Severity at or above `0.75`, or `sulking=true`, produces `PIPELINE_CRITICAL_HAUVIES`. The three-cycle success loop is currently a placeholder; no actual code extrusion or remediation occurs.
- `src/router.c` selects a fixed C or Java starter template. It does not generate a program from the input, compile output, or execute code.
- `include/hauvie.h` defines the public core types and API; the token and router headers expose their respective adapters.

## Input

Example DSL inputs:

```sh
./hauvie-helper --source 'patient=Tech-Agent-404 severity=0.2 sulking=false target=c'
./hauvie-helper --source 'patient=Tech-Agent-404 severity=0.9 sulking=true target=java'
```

Supported fields are:

| Field | Accepted values | Default |
| --- | --- | --- |
| `patient` | 1–31 characters from letters, digits, `_`, `-`, `.` | `Tech-Agent-404` |
| `severity` | finite number from `0.0` to `1.0` | `0.2` |
| `sulking` | `true` or `false` | `false` |
| `target` | `c` or `java` | `c` |

Fields may be supplied in any order. Duplicate or unknown fields, malformed syntax, unsupported characters, and out-of-range values are rejected. The source input is bounded (512 characters, with a bounded token count and token length).

The positional form remains available:

```sh
./hauvie-helper [patient-id [severity]]
```

## AST state contract

`HauvieAst` is a fixed-size, caller-owned parse-result/state object, not a heap-allocated general-language syntax tree. With a non-NULL output object, each parse attempt initializes that object and records a state: `HAUVIE_AST_VERIFIED` plus `HAUVIE_PARSE_ERROR_NONE` on success, or `HAUVIE_AST_REJECTED` plus an error code on invalid input. `HAUVIE_AST_UNVERIFIED` is the initial state during validation. Only verified input is passed to the diagnostic core. A syntactically valid but high-severity input is still verified; severity is a later domain-diagnostic decision and does not invalidate the AST state.

## Core API

Call `hauvie_helper_run(context, patient, report)` from `include/hauvie.h`. It requires a valid context pointer and patient, initializes the context, does not mutate patient data, and returns a `PipelineResult`. The report may be `NULL` and otherwise carries the result and diagnostic metrics. The parser and router are separate APIs for callers that need those stages.

## Base truths and current limits

- Syntax validation and domain diagnosis are separate stages. A critical diagnostic does not turn a verified parse result into a null pointer.
- Syntax errors are rejected before the core diagnostic runs; the AST result object remains available with a rejected state and error code.
- Critical diagnostic reports can still be routed to a starter template, although the CLI exits with failure status for a critical result.
- Severity thresholds and metrics are illustrative project rules, not medically or operationally validated facts.
- There is no AST for a general programming language, no real C/Java generation, and no dynamic compilation or execution.
- The domain-specific cycle behavior and real Hauvies remediation rules remain to be specified and implemented.

## Build and tests

Requires a C11 compiler and Make.

```sh
make
make test
```

`make test` builds and runs the core tests and CLI smoke checks. The diagram source is `docs/pipeline.svg`; a short diagram note is in `docs/pipeline.md`.
