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

/* Parse the small key=value input language into a validated input AST. */
bool hauvie_parse_input(const char *source, HauvieInput *input);

#endif
