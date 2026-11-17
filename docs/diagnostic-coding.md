# Configurable diagnostic coding integration

The library provides a bounded lookup adapter for applications that need to attach corporate-approved ICD-10-CM codes to their own diagnostic keys. It does **not** infer a clinical diagnosis from a pipeline result or provide default code assignments. The team responsible for coding must supply and validate each mapping against the applicable ICD-10-CM release and patient documentation.

## JSON configuration

`include/hauvie_coding.h` exposes `hauvie_coding_map_load_file()` and `hauvie_coding_lookup()`. Configurations use schema version 1, the explicit coding system `ICD-10-CM`, and up to 16 unique application diagnostic keys:

```json
{
  "schema_version": 1,
  "coding_system": "ICD-10-CM",
  "mappings": []
}
```

Corporate-approved mappings are entries in `mappings`, each with a unique `diagnostic_key` and `code`. Keys are application identifiers (ASCII letters, digits, dot, underscore, or hyphen), not patient attributes. The adapter checks bounded size, required fields, duplicate keys, and general ICD-10-CM token shape; it cannot validate that a code is current, billable, or clinically appropriate. An absent mapping returns `false` and no code, so callers should handle it explicitly rather than substitute a default.

Load a replacement file at the application's chosen configuration refresh point (for example, when corporate publishes its monthly update). Loading is transactional: invalid input leaves the destination map empty. The module does not manage file watching, corporate synchronization, rates/fee schedules, or RabbitMQ; code-set mapping and reimbursement rates are distinct concerns. The application can publish a later RabbitMQ event using the code returned by lookup, once the team defines its message contract.

Run the focused checks with `make test-coding` or include them in `make test`.
