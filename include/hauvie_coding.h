#ifndef HAUVIE_CODING_H
#define HAUVIE_CODING_H

#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

#define HAUVIE_CODING_MAX_MAPPINGS 16u
#define HAUVIE_CODING_MAX_KEY_LENGTH 48u
#define HAUVIE_CODING_MAX_CODE_LENGTH 8u
#define HAUVIE_CODING_MAX_JSON_SIZE 4096u

typedef struct {
    char diagnostic_key[HAUVIE_CODING_MAX_KEY_LENGTH + 1u];
    char code[HAUVIE_CODING_MAX_CODE_LENGTH + 1u];
} HauvieCodingEntry;

typedef struct {
    char coding_system[16];
    HauvieCodingEntry entries[HAUVIE_CODING_MAX_MAPPINGS];
    size_t entry_count;
    bool loaded;
} HauvieCodingMap;

/*
 * Load the bounded configuration schema documented in docs/diagnostic-coding.md.
 * Parsing is transactional: on failure, `map` is cleared and no partial map is
 * exposed. ICD-10-CM code shape is checked, but code existence and clinical
 * appropriateness must be validated by the organization's coding authority.
 */
bool hauvie_coding_map_load_json(const char *json, HauvieCodingMap *map);

/* Read one bounded JSON configuration file and load it transactionally. */
bool hauvie_coding_map_load_file(const char *path, HauvieCodingMap *map);

/* Find a configured code for an application-owned diagnostic key. */
bool hauvie_coding_lookup(
    const HauvieCodingMap *map,
    const char *diagnostic_key,
    const char **code
);

#ifdef __cplusplus
}
#endif

#endif
