#include "hauvie_token.h"
#include "hauvie_token_stream.h"

#include <errno.h>
#include <limits.h>
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

static bool token_to_text(const HauvieToken *token, char *text, size_t capacity) {
    if (token->kind != HAUVIE_TOKEN_WORD || token->length == 0 ||
        token->length >= capacity) {
        return false;
    }
    memcpy(text, token->text, token->length);
    text[token->length] = '\0';
    return true;
}

static bool parse_severity(const HauvieToken *token, float *severity) {
    char text[HAUVIE_MAX_TOKEN_LENGTH + 1];
    char *end = NULL;
    float value;

    if (!token_to_text(token, text, sizeof(text))) {
        return false;
    }
    errno = 0;
    value = strtof(text, &end);
    if (errno == ERANGE || end == text || *end != '\0' || !isfinite(value) ||
        value < 0.0f || value > 1.0f) {
        return false;
    }
    *severity = value;
    return true;
}

static bool parse_receiver_number(const HauvieToken *token, double *number,
                                 bool unit_interval) {
    char text[HAUVIE_MAX_TOKEN_LENGTH + 1];
    char *end = NULL;
    double value;

    if (!token_to_text(token, text, sizeof(text))) {
        return false;
    }
    errno = 0;
    value = strtod(text, &end);
    if (errno == ERANGE || end == text || *end != '\0' || !isfinite(value) ||
        value < 0.0 || (unit_interval && value > 1.0)) {
        return false;
    }
    *number = value;
    return true;
}

static bool parse_receiver_context(const HauvieToken *token,
                                   unsigned int *context_id) {
    char text[HAUVIE_MAX_TOKEN_LENGTH + 1];
    char *end = NULL;
    unsigned long value;

    if (!token_to_text(token, text, sizeof(text))) {
        return false;
    }
    for (size_t i = 0; text[i] != '\0'; ++i) {
        if (text[i] < '0' || text[i] > '9') {
            return false;
        }
    }
    errno = 0;
    value = strtoul(text, &end, 10);
    if (errno == ERANGE || end == text || *end != '\0' || value > UINT_MAX) {
        return false;
    }
    *context_id = (unsigned int)value;
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
        .target = HAUVIE_TARGET_C,
        .receiver_analysis_enabled = false,
        .receiver_context_id = 0,
        .receiver_left_output = 0.0,
        .receiver_right_output = 0.0,
        .receiver_tolerance = 0.0
    };
    bool seen_patient = false;
    bool seen_severity = false;
    bool seen_sulking = false;
    bool seen_target = false;
    bool seen_receiver_context = false;
    bool seen_receiver_left = false;
    bool seen_receiver_right = false;
    bool seen_receiver_tolerance = false;
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
        } else if (token_is_word(&key, "receiver_context")) {
            if (seen_receiver_context || !parse_receiver_context(
                    &value, &parsed.receiver_context_id)) {
                return false;
            }
            seen_receiver_context = true;
        } else if (token_is_word(&key, "receiver_left")) {
            if (seen_receiver_left || !parse_receiver_number(
                    &value, &parsed.receiver_left_output, true)) {
                return false;
            }
            seen_receiver_left = true;
        } else if (token_is_word(&key, "receiver_right")) {
            if (seen_receiver_right || !parse_receiver_number(
                    &value, &parsed.receiver_right_output, true)) {
                return false;
            }
            seen_receiver_right = true;
        } else if (token_is_word(&key, "receiver_tolerance")) {
            if (seen_receiver_tolerance || !parse_receiver_number(
                    &value, &parsed.receiver_tolerance, false)) {
                return false;
            }
            seen_receiver_tolerance = true;
        } else {
            return false;
        }
    }

    bool any_receiver_field = seen_receiver_context || seen_receiver_left ||
        seen_receiver_right || seen_receiver_tolerance;
    bool complete_receiver_fields = seen_receiver_context && seen_receiver_left &&
        seen_receiver_right && seen_receiver_tolerance;
    if (!saw_field || (any_receiver_field && !complete_receiver_fields) ||
        !hauvies_patient_is_valid(&parsed.patient)) {
        return false;
    }
    parsed.receiver_analysis_enabled = complete_receiver_fields;
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
            .target = HAUVIE_TARGET_C,
            .receiver_analysis_enabled = false
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
