# Hauvie Helper Repair

A small C project that models a Hauvie diagnostic pipeline. It currently provides a bounded input DSL, a deterministic diagnostic stub, and fixed C/Java starter-template output. It is a scaffold for defining the real code-extruder behavior—not a general-purpose source-code parser, compiler, or production diagnostic system.

## Current pipeline

![Hauvie helper pipeline architecture](docs/pipeline.svg)

```text
CLI input -> token.c -> HauvieInput -> hauvie.c -> PipelineReport -> router.c -> C/Java starter template
```

- `src/main.c` is the CLI entry point. It accepts either positional patient/severity arguments or `--source` input.
- `src/token.c` parses a limited sequence of whitespace-separated `key=value` fields. This is a small configuration DSL, not a lexer/parser for C, Java, or arbitrary submitted code.
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

## Core API

Call `hauvie_helper_run(context, patient, report)` from `include/hauvie.h`. It requires a valid context pointer and patient, initializes the context, does not mutate patient data, and returns a `PipelineResult`. The report may be `NULL` and otherwise carries the result and diagnostic metrics. The parser and router are separate APIs for callers that need those stages.

## Base truths and current limits

- A syntactically valid input can still be diagnostically critical; critical severity does not invalidate the parsed `HauvieInput`.
- Syntax errors are rejected before the core diagnostic runs.
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
