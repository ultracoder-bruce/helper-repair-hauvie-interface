# Finite Receiver Model Boundary

The receiver model is a standalone, bounded comparison API. It is deliberately
separate from the patient-specific `patient=... severity=...` DSL and from the
Hauvie diagnostic pipeline. It does not change `HauvieInput`, parser behavior,
or patient validation.

`HauvieReceiverSample` records up to eight finite scalar outputs from a named
(integer-ID) receiver context. Two samples are equivalent only when they use
the same context, have the same number of outputs, and every corresponding
output differs by no more than the caller-supplied absolute tolerance. A
context mismatch is reported explicitly rather than silently compared.

This first increment is a classical finite-output abstraction for receiver
equivalence. It has no density-matrix representation, quantum-state simulator,
transformation/congruence engine, or physical interpretation. In particular,
it does not establish experimental results or claims about time reversal.

Build and run its focused tests with:

```sh
make test-receiver
./test-receiver
```
