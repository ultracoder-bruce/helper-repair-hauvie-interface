# Receiver-aware diagnostic input

The patient DSL remains patient-specific: `patient`, `severity`, `sulking`, and
`target` retain their existing meanings. The bounded source parser also accepts
an optional, explicitly namespaced receiver-analysis group:

```text
receiver_context=7 receiver_left=0.45 receiver_right=0.4505 receiver_tolerance=0.001
```

All four receiver fields are required together. Context is a non-negative
integer; the two scalar outputs are finite values in `[0, 1]`; tolerance is a
finite non-negative value. Duplicate, incomplete, out-of-range, or malformed
fields reject the whole input. Without the receiver group, existing patient
input behavior is unchanged.

The diagnostic entry point runs the existing patient pipeline and then
compares the two scalar outputs under the same context using the configured
absolute tolerance. It reports either equivalent or distinguishable for that
finite receiver comparison, separately from the patient pipeline result. The
receiver fields are not patient properties and do not modify patient severity,
validation, or the pipeline's adjustment rules.

This is a finite software model only. It does not use density matrices, model
quantum dynamics, establish experimental findings, or imply physical time
reversal. Build and run the integration checks with `make test-diagnostic`.
