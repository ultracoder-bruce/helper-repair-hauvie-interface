# Diagnostic remediation policy

`hauvie_remediation_for_result` is a policy lookup; it does not change diagnostic results, run a repair, or format generated source. It intentionally defines only the behavior supported by the project:

| Diagnostic result | Remediation policy |
| --- | --- |
| `PIPELINE_OK` | `HAUVIE_REMEDIATION_NONE` |
| `PIPELINE_CRITICAL_HAUVIES` | `HAUVIE_REMEDIATION_REVIEW` |
| `PIPELINE_INVALID_ARGUMENT` | `HAUVIE_REMEDIATION_UNSPECIFIED` |
| `PIPELINE_INFRASTRUCTURE_FAILURE` | `HAUVIE_REMEDIATION_UNSPECIFIED` |

Unspecified (including future/unknown) results do not imply an action. This matches the existing repair-queue integration, which queues a bounded review request only for a critical patient result; provider callbacks remain responsible for any external work. Diagnostic classification remains in the diagnostic pipeline, and generated-code rendering remains in the router.

## Bounded output buffer

`hauvie_router_render_buffer` writes the existing fixed C or Java template into caller-owned storage. `required` and `written` count bytes excluding the NUL terminator. A sizing call with `buffer == NULL` and `capacity == 0` reports `required`; an undersized nonzero buffer receives a truncated but NUL-terminated prefix, reports the number of bytes stored in `written`, and still returns true. The caller can retry with at least `required + 1` bytes for the complete template. Invalid arguments and unsupported targets return false.

Run focused tests with `make test-router-remediation`; `make test` includes the same tests.
