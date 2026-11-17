#ifndef HAUVIE_CODEGEN_H
#define HAUVIE_CODEGEN_H

#include "hauvie.h"
#include "hauvie_token.h"

#include <stddef.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/* A caller-owned byte sink; writes may be delivered in multiple chunks. */
typedef bool (*HauvieCodegenWrite)(void *context, const char *data,
                                   size_t length);

typedef struct {
    HauvieCodegenWrite write;
    void *context;
} HauvieCodegenSink;

/* Visit the report with a C or Java generator and emit source to the sink. */
bool hauvie_codegen_generate(const HauvieCodegenSink *sink,
                             HauvieTarget target,
                             const PipelineReport *report);

#ifdef __cplusplus
}
#endif

#endif
