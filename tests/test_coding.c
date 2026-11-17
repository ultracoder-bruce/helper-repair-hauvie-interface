#include "hauvie_coding.h"

#include <assert.h>
#include <stdio.h>
#include <string.h>

static void test_load_and_lookup(void) {
    const char *json =
        "{\"schema_version\":1,\"coding_system\":\"ICD-10-CM\","
        "\"mappings\":[{\"diagnostic_key\":\"monitor.case_critical\","
        "\"code\":\"S06.0XAA\"}]}";
    HauvieCodingMap map;
    const char *code = NULL;

    assert(hauvie_coding_map_load_json(json, &map));
    assert(map.loaded && map.entry_count == 1u);
    assert(hauvie_coding_lookup(&map, "monitor.case_critical", &code));
    assert(strcmp(code, "S06.0XAA") == 0);
    assert(!hauvie_coding_lookup(&map, "monitor.unconfigured", &code));
    assert(code == NULL);
}

static void test_schema_order_and_empty_mapping(void) {
    HauvieCodingMap map;
    assert(hauvie_coding_map_load_json(
        "{\"mappings\":[], \"coding_system\":\"ICD-10-CM\","
        "\"schema_version\":1}", &map));
    assert(map.loaded && map.entry_count == 0u);
}

static void test_rejects_bad_config_transactionally(void) {
    HauvieCodingMap map;
    const char *code = (const char *)1;

    assert(hauvie_coding_map_load_json(
        "{\"schema_version\":1,\"coding_system\":\"ICD-10-CM\","
        "\"mappings\":[{\"diagnostic_key\":\"x\",\"code\":\"A00\"},"
        "{\"diagnostic_key\":\"x\",\"code\":\"A01\"}]}", &map) == false);
    assert(!map.loaded && map.entry_count == 0u);
    assert(!hauvie_coding_lookup(&map, "x", &code));
    assert(code == NULL);
    assert(!hauvie_coding_map_load_json(
        "{\"schema_version\":1,\"coding_system\":\"ICD-10\","
        "\"mappings\":[]}", &map));
    assert(!hauvie_coding_map_load_json(
        "{\"schema_version\":1,\"coding_system\":\"ICD-10-CM\","
        "\"mappings\":[{\"diagnostic_key\":\"x\",\"code\":\"not-code\"}]}",
        &map));
}

static void test_rejects_json_extensions_and_invalid_lookups(void) {
    HauvieCodingMap map;
    const char *code = NULL;
    assert(hauvie_coding_map_load_json(
        "{\"schema_version\":1,\"coding_system\":\"ICD-10-CM\","
        "\"mappings\":[]}", &map));
    assert(!hauvie_coding_lookup(&map, "bad key", &code));
    assert(!hauvie_coding_lookup(&map, "x", NULL));
    assert(!hauvie_coding_map_load_json("{} trailing", &map));
}

int main(void) {
    test_load_and_lookup();
    test_schema_order_and_empty_mapping();
    test_rejects_bad_config_transactionally();
    test_rejects_json_extensions_and_invalid_lookups();
    puts("Diagnostic coding configuration tests passed.");
    return 0;
}
