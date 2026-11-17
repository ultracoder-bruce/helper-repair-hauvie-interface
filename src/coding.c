#include "hauvie_coding.h"

#include <stdio.h>
#include <string.h>

typedef struct {
    const char *cursor;
} JsonParser;

static void skip_space(JsonParser *parser) {
    while (*parser->cursor == ' ' || *parser->cursor == '\t' ||
           *parser->cursor == '\n' || *parser->cursor == '\r') {
        ++parser->cursor;
    }
}

static bool consume(JsonParser *parser, char expected) {
    skip_space(parser);
    if (*parser->cursor != expected) {
        return false;
    }
    ++parser->cursor;
    return true;
}

static bool parse_string(JsonParser *parser, char *out, size_t capacity) {
    size_t length = 0;

    if (!consume(parser, '"') || capacity == 0) {
        return false;
    }
    while (*parser->cursor != '\0' && *parser->cursor != '"') {
        unsigned char value = (unsigned char)*parser->cursor++;
        if (value == '\\') {
            char escaped = *parser->cursor++;
            switch (escaped) {
                case '"': value = '"'; break;
                case '\\': value = '\\'; break;
                case '/': value = '/'; break;
                case 'b': value = '\b'; break;
                case 'f': value = '\f'; break;
                case 'n': value = '\n'; break;
                case 'r': value = '\r'; break;
                case 't': value = '\t'; break;
                default:
                    /* Configuration identifiers are ASCII; Unicode escapes
                     * are intentionally not accepted by this small parser. */
                    return false;
            }
        } else if (value < 0x20u) {
            return false;
        }
        if (value == '\0' || length + 1u >= capacity) {
            return false;
        }
        out[length++] = (char)value;
    }
    if (*parser->cursor != '"') {
        return false;
    }
    ++parser->cursor;
    out[length] = '\0';
    return true;
}

static bool valid_key(const char *key) {
    size_t i;
    if (key[0] == '\0') {
        return false;
    }
    for (i = 0; key[i] != '\0'; ++i) {
        char c = key[i];
        if (!((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') ||
              (c >= '0' && c <= '9') || c == '_' || c == '.' || c == '-')) {
            return false;
        }
    }
    return true;
}

/* Check the general ICD-10-CM token shape, not whether it is an assigned code. */
static bool valid_code(const char *code) {
    size_t i;
    size_t length = strlen(code);
    size_t dot_count = 0;

    if (length < 3u || length > HAUVIE_CODING_MAX_CODE_LENGTH ||
        code[0] < 'A' || code[0] > 'Z' ||
        code[1] < '0' || code[1] > '9') {
        return false;
    }
    for (i = 2; i < length; ++i) {
        char c = code[i];
        if (c == '.') {
            if (++dot_count > 1u || i != 3u || i + 1u == length) {
                return false;
            }
        } else if (!((c >= 'A' && c <= 'Z') ||
                     (c >= '0' && c <= '9'))) {
            return false;
        }
    }
    return true;
}

static bool parse_version(JsonParser *parser) {
    skip_space(parser);
    if (*parser->cursor != '1') {
        return false;
    }
    ++parser->cursor;
    return *parser->cursor < '0' || *parser->cursor > '9';
}

static bool parse_mapping(JsonParser *parser, HauvieCodingMap *map) {
    char key[HAUVIE_CODING_MAX_KEY_LENGTH + 1u] = {0};
    char code[HAUVIE_CODING_MAX_CODE_LENGTH + 1u] = {0};
    unsigned int seen = 0;

    if (!consume(parser, '{')) {
        return false;
    }
    for (;;) {
        char field[24];
        if (!parse_string(parser, field, sizeof(field)) ||
            !consume(parser, ':')) {
            return false;
        }
        if (strcmp(field, "diagnostic_key") == 0) {
            if ((seen & 1u) != 0u ||
                !parse_string(parser, key, sizeof(key))) {
                return false;
            }
            seen |= 1u;
        } else if (strcmp(field, "code") == 0) {
            if ((seen & 2u) != 0u ||
                !parse_string(parser, code, sizeof(code))) {
                return false;
            }
            seen |= 2u;
        } else {
            return false;
        }
        skip_space(parser);
        if (*parser->cursor == '}') {
            ++parser->cursor;
            break;
        }
        if (!consume(parser, ',')) {
            return false;
        }
    }
    if (seen != 3u || !valid_key(key) || !valid_code(code) ||
        map->entry_count >= HAUVIE_CODING_MAX_MAPPINGS) {
        return false;
    }
    for (size_t i = 0; i < map->entry_count; ++i) {
        if (strcmp(map->entries[i].diagnostic_key, key) == 0) {
            return false;
        }
    }
    memcpy(map->entries[map->entry_count].diagnostic_key, key, strlen(key) + 1u);
    memcpy(map->entries[map->entry_count].code, code, strlen(code) + 1u);
    ++map->entry_count;
    return true;
}

static bool parse_mappings(JsonParser *parser, HauvieCodingMap *map) {
    if (!consume(parser, '[')) {
        return false;
    }
    skip_space(parser);
    if (*parser->cursor == ']') {
        ++parser->cursor;
        return true;
    }
    for (;;) {
        if (!parse_mapping(parser, map)) {
            return false;
        }
        skip_space(parser);
        if (*parser->cursor == ']') {
            ++parser->cursor;
            return true;
        }
        if (!consume(parser, ',')) {
            return false;
        }
    }
}

bool hauvie_coding_map_load_json(const char *json, HauvieCodingMap *map) {
    HauvieCodingMap parsed = {0};
    JsonParser parser;
    unsigned int seen = 0;
    size_t length;

    if (map == NULL) {
        return false;
    }
    *map = (HauvieCodingMap){0};
    if (json == NULL) {
        return false;
    }
    length = strlen(json);
    if (length == 0u || length > HAUVIE_CODING_MAX_JSON_SIZE) {
        return false;
    }
    parser.cursor = json;
    if (!consume(&parser, '{')) {
        return false;
    }
    for (;;) {
        char field[24];
        if (!parse_string(&parser, field, sizeof(field)) ||
            !consume(&parser, ':')) {
            return false;
        }
        if (strcmp(field, "schema_version") == 0) {
            if ((seen & 1u) != 0u || !parse_version(&parser)) {
                return false;
            }
            seen |= 1u;
        } else if (strcmp(field, "coding_system") == 0) {
            if ((seen & 2u) != 0u ||
                !parse_string(&parser, parsed.coding_system,
                              sizeof(parsed.coding_system))) {
                return false;
            }
            seen |= 2u;
        } else if (strcmp(field, "mappings") == 0) {
            if ((seen & 4u) != 0u || !parse_mappings(&parser, &parsed)) {
                return false;
            }
            seen |= 4u;
        } else {
            return false;
        }
        skip_space(&parser);
        if (*parser.cursor == '}') {
            ++parser.cursor;
            break;
        }
        if (!consume(&parser, ',')) {
            return false;
        }
    }
    skip_space(&parser);
    if (*parser.cursor != '\0' || seen != 7u ||
        strcmp(parsed.coding_system, "ICD-10-CM") != 0) {
        return false;
    }
    parsed.loaded = true;
    *map = parsed;
    return true;
}

bool hauvie_coding_map_load_file(const char *path, HauvieCodingMap *map) {
    char buffer[HAUVIE_CODING_MAX_JSON_SIZE + 1u];
    FILE *file;
    size_t length;
    bool valid;

    if (map == NULL) {
        return false;
    }
    *map = (HauvieCodingMap){0};
    if (path == NULL || path[0] == '\0') {
        return false;
    }
    file = fopen(path, "rb");
    if (file == NULL) {
        return false;
    }
    length = fread(buffer, 1u, sizeof(buffer), file);
    valid = !ferror(file) && length <= HAUVIE_CODING_MAX_JSON_SIZE &&
            feof(file);
    if (fclose(file) != 0) {
        valid = false;
    }
    if (!valid) {
        return false;
    }
    buffer[length] = '\0';
    return hauvie_coding_map_load_json(buffer, map);
}

bool hauvie_coding_lookup(
    const HauvieCodingMap *map,
    const char *diagnostic_key,
    const char **code
) {
    if (code != NULL) {
        *code = NULL;
    }
    if (map == NULL || !map->loaded || diagnostic_key == NULL ||
        code == NULL || !valid_key(diagnostic_key)) {
        return false;
    }
    for (size_t i = 0; i < map->entry_count; ++i) {
        if (strcmp(map->entries[i].diagnostic_key, diagnostic_key) == 0) {
            *code = map->entries[i].code;
            return true;
        }
    }
    return false;
}
