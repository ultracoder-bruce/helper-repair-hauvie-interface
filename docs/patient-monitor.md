# Patient loop monitoring contract

`hauvie_monitor.h` adds an optional synchronous callback to `hauvies_helper_run_monitored()`. For a successful patient run it emits one `HAUVIE_MONITOR_CYCLE_COMPLETED` snapshot per fixed pipeline cycle, then one `HAUVIE_MONITOR_DIAGNOSTIC_COMPLETED` event. Critical and invalid inputs emit a final diagnostic event without cycle events. The legacy `hauvies_helper_run()` uses the same pipeline with monitoring disabled.

Cycle progress is a count (`report.cycles_completed`), not a rate. The pipeline has a fixed, deterministic number of computational cycles; it has no patient-velocity input, sample timestamps, or wall-clock loop cadence. `raw_binary_volume` influences the modeled volumetric-efficiency adjustment, not iterations per second. The callback layer does not measure or imply patient movement/flow velocity, clinical observations, or real-time performance. If an application needs processing throughput, it should measure elapsed time around the call using its own monotonic clock and keep that operational metric separate from clinical coding. Do not label pipeline iteration rate as patient velocity.

Callbacks run inline on the calling thread. They must be bounded, nonblocking, and must not re-enter the patient pipeline. Copy the event snapshot during the callback; event strings and the event pointer are borrowed. A configured code is only an application mapping for a diagnostic key and is not an automatically generated or validated clinical diagnosis. Never use fixture mappings for care, claims, or production billing. Keep patient identity out of queue/log payloads unless the application has appropriate authorization, minimization, and safeguards.

The callback module contains no RabbitMQ client or wire format. A later transport adapter can enqueue a copied, minimized event to a caller-owned event loop once message schema, delivery, retry, and PHI policies are defined.

Build and run its fixture checks with:

```sh
make test-monitor test-coding
./test-monitor
./test-coding
```
