# Hauvie Helper Repair

The project is expanding into a small, testable code-extruder pipeline with distinct ingestion, token/AST, core diagnostic, and output-routing stages.

## Pipeline

```text
main.c -> token.c -> HauvieInput AST -> hauvie.c -> PipelineReport -> router.c
```

- `src/main.c` is the entry hub. It accepts the legacy diagnostic arguments or a `--source` DSL input.
- `src/token.c` lexes and parses a bounded `key=value` input format into a `HauvieInput` AST. Syntax validation is separate from semantic severity handling: a high-severity but well-formed input retains a valid AST and produces a critical diagnostic report rather than a null AST.
- `src/hauvie.c` contains the reusable core logic. Patient data is read-only; critical status is explicit in the report.
- `src/router.c` selects a fixed C or Java output template. Routing is based on the requested target and remains available for critical reports, so sulking does not erase the target/template selection.

## Input format

```sh
./hauvie-helper --source 'patient=Tech-Agent-404 severity=0.2 sulking=false target=c'
./hauvie-helper --source 'patient=Tech-Agent-404 severity=0.9 sulking=true target=java'
```

Supported fields are `patient`, `severity` (finite number from `0.0` to `1.0`), `sulking` (`true` or `false`), and `target` (`c` or `java`). Fields are optional and have safe defaults. Unknown/duplicate fields, malformed tokens, oversized input, and invalid values are rejected. The router emits starter templates only; it does not compile or execute submitted source code.

## Public core API

`include/hauvie.h` defines the patient, context, result, and report types. `hauvie_helper_run(context, patient, report)` runs the diagnostic independently from parsing and presentation. `include/hauvie_token.h` and `include/hauvie_router.h` expose the parser and output adapter for other interfaces.

## Build and test

```sh
make
make test
```
