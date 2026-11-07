# Hauvie Helper Repair

The project is being expanded toward a reusable core-logic interface. The C library/API is separated from presentation so a CLI, web UI, or other front end can call the same deterministic behavior.

## Project layout

- `include/hauvie.h` — public types, result codes, and callable API.
- `src/hauvie.c` — core validation, initialization, and diagnostic logic. It does not print output or mutate patient data.
- `src/main.c` — command-line presentation adapter.
- `tests/test_hauvie.c` — direct tests of the core API.

## Core interface

Call `hauvie_helper_run(context, patient, report)` with a valid patient and context. It returns a `PipelineResult` and, when provided, fills `PipelineReport` with a stable result suitable for UI rendering. Severity must be finite and in the range `0.0`–`1.0`; severity at or above `0.75`, or a sulking state, returns `PIPELINE_CRITICAL_HAUVIES`. Patient data is read-only to the core routine.

```c
#include "hauvie.h"

ExtruderContext context;
PipelineReport report;
HauviesPatient patient = {
    .individual_id = "Tech-Agent-404",
    .hauvies_severity = 0.2f,
    .is_sulking = false,
    .raw_binary_volume = 4096
};

PipelineResult result = hauvies_helper_run(&context, &patient, &report);
```

## Build and test

```sh
make
./hauvie-helper [patient-id [severity]]
make test
```

The core is intentionally small at this stage: cycle processing is a placeholder until the actual domain rules are specified. The public API establishes the seam where that logic can be implemented without coupling it to CLI output.
