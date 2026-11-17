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
    /* Optional scalar receiver comparison; kept distinct from patient data. */
    bool receiver_analysis_enabled;
    unsigned int receiver_context_id;
    double receiver_left_output;
    double receiver_right_output;
    double receiver_tolerance;
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

/* A non-heap parse result/state container with explicit verification status. */
typedef struct {
    HauvieAstStatus status;
    HauvieParseError error;
    HauvieInput input;
} HauvieAst;

/* Parse bounded patient key=value fields and optional receiver_* fields. */
bool hauvie_parse_ast(const char *source, HauvieAst *ast);

/* Compatibility helper: parse directly into a validated input value. */
bool hauvie_parse_input(const char *source, HauvieInput *input);

#endif
