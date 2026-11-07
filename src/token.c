#include "hauvie_token.h"
#include "hauvie_token_stream.h"

#include <errno.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>

static bool next_token(HauvieTokenStream *stream, HauvieToken *token) {
    return hauvie_token_stream_next(stream, token) == HAUVIE_STREAM_TOKEN;
}

static bool token_is_word(const HauvieToken *token, const char *expected) {
    size_t length = strlen(expected);
    return token->kind == HAUVIE_TOKEN_WORD && token->length == length &&
           memcmp(token->text, expected, length) == 0;
}

static bool parse_severity(const HauvieToken *token, float *severity) {
    char text[HAUVIE_MAX_TOKEN_LENGTH + 1];
    char *end = NULL;
    float value;

    if (token->kind != HAUVIE_TOKEN_WORD || token->length == 0 ||
        token->length > HAUVIE_MAX_TOKEN_LENGTH) {
        return false;
    }
    memcpy(text, token->text, token->length);
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

static bool copy_patient_id(char *destination, size_t capacity,
                            const HauvieToken *token) {
    if (token->kind != HAUVIE_TOKEN_WORD || token->length == 0 ||
        token->length >= capacity) {
        return false;
    }
    memcpy(destination, token->text, token->length);
    destination[token->length] = '\0';
    return true;
}

bool hauvie_parse_input(const char *source, HauvieInput *input) {
    HauvieTokenStream stream;
    HauvieInput parsed = {
        .patient = {
            .individual_id = "Tech-Agent-404",
            .hauvies_severity = 0.2f,
            .is_sulking = false,
            .raw_binary_volume = 4096
        },
        .target = HAUVIE_TARGET_C
    };
    bool seen_patient = false;
    bool seen_severity = false;
    bool seen_sulking = false;
    bool seen_target = false;
    bool saw_field = false;

    if (input == NULL || !hauvie_token_stream_init(&stream, source)) {
        return false;
    }

    for (;;) {
        HauvieToken key;
        HauvieToken equals;
        HauvieToken value;
        HauvieStreamResult result = hauvie_token_stream_next(&stream, &key);
        if (result == HAUVIE_STREAM_END) {
            break;
        }
        if (result != HAUVIE_STREAM_TOKEN || key.kind != HAUVIE_TOKEN_WORD ||
            !next_token(&stream, &equals) || equals.kind != HAUVIE_TOKEN_EQUALS ||
            !next_token(&stream, &value) || value.kind != HAUVIE_TOKEN_WORD) {
            return false;
        }
        saw_field = true;

        if (token_is_word(&key, "patient")) {
            if (seen_patient || !copy_patient_id(parsed.patient.individual_id,
                    sizeof(parsed.patient.individual_id), &value)) {
                return false;
            }
            seen_patient = true;
        } else if (token_is_word(&key, "severity")) {
            if (seen_severity || !parse_severity(&value,
                    &parsed.patient.hauvies_severity)) {
                return false;
            }
            seen_severity = true;
        } else if (token_is_word(&key, "sulking")) {
            if (seen_sulking) {
                return false;
            }
            if (token_is_word(&value, "true")) {
                parsed.patient.is_sulking = true;
            } else if (token_is_word(&value, "false")) {
                parsed.patient.is_sulking = false;
            } else {
                return false;
            }
            seen_sulking = true;
        } else if (token_is_word(&key, "target")) {
            if (seen_target) {
                return false;
            }
            if (token_is_word(&value, "c")) {
                parsed.target = HAUVIE_TARGET_C;
            } else if (token_is_word(&value, "java")) {
                parsed.target = HAUVIE_TARGET_JAVA;
            } else {
                return false;
            }
            seen_target = true;
        } else {
            return false;
        }
    }

    if (!saw_field || !hauvies_patient_is_valid(&parsed.patient)) {
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
