#ifndef HAUVIE_TOKEN_H
#define HAUVIE_TOKEN_H

#include "hauvie.h"

#include <stdbool.h>
#include <stddef.h>

#define HAUVIE_MAX_SOURCE_TOKENS 32

typedef enum {
    HAUVIE_TARGET_C = 0,
    HAUVIE_TARGET_JAVA
} HauvieTarget;

typedef struct {
    HauviesPatient patient;
    HauvieTarget target;
} HauvieInput;

typedef enum {
    HAUVIE_AST_UNVERIFIED = 0,
    HAUVIE_AST_VERIFIED,
    HAUVIE_AST_REJECTED
} HauvieAstStatus;

typedef enum {
    HAUVIE_PARSE_ERROR_NONE = 0,
    HAUVIE_PARSE_ERROR_INVALID_INPUT
} HauvieParseError;

/*
 * A non-heap parse result/state container. Every parse attempt with a non-NULL
 * output receives a state: VERIFIED on success, REJECTED on invalid input.
 */
typedef struct {
    HauvieAstStatus status;
    HauvieParseError error;
    HauvieInput input;
} HauvieAst;

/* Parse the bounded key=value language into an explicit AST state. */
bool hauvie_parse_ast(const char *source, HauvieAst *ast);

/* Compatibility helper: parse directly into a validated input value. */
bool hauvie_parse_input(const char *source, HauvieInput *input);

#endif
