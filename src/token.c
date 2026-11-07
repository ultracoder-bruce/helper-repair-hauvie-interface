#include "hauvie_token.h"

#include <errno.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>

#define MAX_SOURCE_LENGTH 512
#define MAX_WORD_LENGTH 64

typedef enum {
    TOKEN_WORD,
    TOKEN_EQUALS
} TokenKind;

typedef struct {
    TokenKind kind;
    size_t start;
    size_t length;
} Token;

static bool is_word_character(char ch) {
    return (ch >= 'a' && ch <= 'z') ||
           (ch >= 'A' && ch <= 'Z') ||
           (ch >= '0' && ch <= '9') ||
           ch == '_' || ch == '-' || ch == '.';
}

static bool tokenize(const char *source, Token *tokens, size_t capacity,
                     size_t *token_count) {
    size_t length;
    size_t position = 0;
    size_t count = 0;

    if (source == NULL || tokens == NULL || token_count == NULL) {
        return false;
    }
    length = strlen(source);
    if (length == 0 || length > MAX_SOURCE_LENGTH) {
        return false;
    }

    while (position < length) {
        if (source[position] == ' ' || source[position] == '\t' ||
            source[position] == '\n' || source[position] == '\r') {
            ++position;
            continue;
        }
        if (count == capacity) {
            return false;
        }
        if (source[position] == '=') {
            tokens[count++] = (Token){TOKEN_EQUALS, position, 1};
            ++position;
            continue;
        }
        if (!is_word_character(source[position])) {
            return false;
        }

        size_t start = position;
        while (position < length && is_word_character(source[position])) {
            ++position;
        }
        if (position - start > MAX_WORD_LENGTH) {
            return false;
        }
        tokens[count++] = (Token){TOKEN_WORD, start, position - start};
    }

    *token_count = count;
    return count > 0;
}

static bool word_equals(const char *source, const Token *token,
                        const char *expected) {
    size_t expected_length = strlen(expected);
    return token->kind == TOKEN_WORD && token->length == expected_length &&
           memcmp(source + token->start, expected, expected_length) == 0;
}

static bool parse_severity_word(const char *source, const Token *token,
                                float *severity) {
    char text[MAX_WORD_LENGTH + 1];
    char *end = NULL;
    float value;

    if (token->kind != TOKEN_WORD || token->length == 0 ||
        token->length > MAX_WORD_LENGTH) {
        return false;
    }
    memcpy(text, source + token->start, token->length);
    text[token->length] = '\0';
    errno = 0;
    value = strtof(text, &end);
    if (errno == ERANGE || end == text || *end != '\0' || !isfinite(value) ||
        value < 0.0f || value > 1.0f) {
        return false;
    }
    *severity = value;
    return true;
}

static bool copy_word(char *destination, size_t destination_size,
                      const char *source, const Token *token) {
    if (token->kind != TOKEN_WORD || token->length == 0 ||
        token->length >= destination_size) {
        return false;
    }
    memcpy(destination, source + token->start, token->length);
    destination[token->length] = '\0';
    return true;
}

bool hauvie_parse_input(const char *source, HauvieInput *input) {
    Token tokens[HAUVIE_MAX_SOURCE_TOKENS];
    size_t token_count = 0;
    bool seen_patient = false;
    bool seen_severity = false;
    bool seen_sulking = false;
    bool seen_target = false;
    HauvieInput parsed = {
        .patient = {
            .individual_id = "Tech-Agent-404",
            .hauvies_severity = 0.2f,
            .is_sulking = false,
            .raw_binary_volume = 4096
        },
        .target = HAUVIE_TARGET_C
    };

    if (input == NULL || !tokenize(source, tokens,
                                   HAUVIE_MAX_SOURCE_TOKENS, &token_count) ||
        token_count % 3 != 0) {
        return false;
    }

    for (size_t i = 0; i < token_count; i += 3) {
        const Token *key = &tokens[i];
        const Token *equals = &tokens[i + 1];
        const Token *value = &tokens[i + 2];

        if (key->kind != TOKEN_WORD || equals->kind != TOKEN_EQUALS ||
            value->kind != TOKEN_WORD) {
            return false;
        }
        if (word_equals(source, key, "patient")) {
            if (seen_patient || !copy_word(parsed.patient.individual_id,
                                           sizeof(parsed.patient.individual_id),
                                           source, value)) {
                return false;
            }
            seen_patient = true;
        } else if (word_equals(source, key, "severity")) {
            if (seen_severity || !parse_severity_word(source, value,
                                                       &parsed.patient.hauvies_severity)) {
                return false;
            }
            seen_severity = true;
        } else if (word_equals(source, key, "sulking")) {
            if (seen_sulking) {
                return false;
            }
            if (word_equals(source, value, "true")) {
                parsed.patient.is_sulking = true;
            } else if (word_equals(source, value, "false")) {
                parsed.patient.is_sulking = false;
            } else {
                return false;
            }
            seen_sulking = true;
        } else if (word_equals(source, key, "target")) {
            if (seen_target) {
                return false;
            }
            if (word_equals(source, value, "c")) {
                parsed.target = HAUVIE_TARGET_C;
            } else if (word_equals(source, value, "java")) {
                parsed.target = HAUVIE_TARGET_JAVA;
            } else {
                return false;
            }
            seen_target = true;
        } else {
            return false;
        }
    }

    if (!hauvies_patient_is_valid(&parsed.patient)) {
        return false;
    }
    *input = parsed;
    return true;
}

bool hauvie_parse_ast(const char *source, HauvieAst *ast) {
    HauvieInput parsed;

    if (ast == NULL) {
        return false;
    }

    *ast = (HauvieAst){
        .status = HAUVIE_AST_UNVERIFIED,
        .error = HAUVIE_PARSE_ERROR_INVALID_INPUT,
        .input = {
            .patient = {
                .individual_id = "Tech-Agent-404",
                .hauvies_severity = 0.2f,
                .is_sulking = false,
                .raw_binary_volume = 4096
            },
            .target = HAUVIE_TARGET_C
        }
    };

    if (!hauvie_parse_input(source, &parsed)) {
        ast->status = HAUVIE_AST_REJECTED;
        return false;
    }

    ast->input = parsed;
    ast->error = HAUVIE_PARSE_ERROR_NONE;
    ast->status = HAUVIE_AST_VERIFIED;
    return true;
}
