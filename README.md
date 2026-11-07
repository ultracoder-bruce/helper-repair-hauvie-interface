# Hauvie Helper Repair

A small C diagnostic program for the Hauvies helper pipeline. The implementation keeps the original mock extruder terminology while making the diagnostic predictable and safe to call.

## Build and run

```sh
make
./hauvie-helper
./hauvie-helper Tech-Agent-404 0.2
```

The optional severity must be between `0.0` and `1.0`; values at or above `0.75` are reported as critical. The diagnostic does not mutate patient data or gradually increase severity as a side effect. A critical patient exits unsuccessfully with a clear status, while a non-critical patient completes the three-cycle pipeline.

Run the smoke tests with:

```sh
make test
```
