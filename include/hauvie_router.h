#ifndef HAUVIE_ROUTER_H
#define HAUVIE_ROUTER_H

#include "hauvie.h"
#include "hauvie_token.h"

#include <stddef.h>
#include <stdio.h>

/* Generate C or Java source directly to the supplied stream. */
bool hauvie_router_render(FILE *output, HauvieTarget target,
                          const PipelineReport *report);

/*
 * Generate into caller-owned storage. `required` receives the number of output
 * bytes excluding the NUL terminator. `written` receives bytes actually stored.
 * If capacity is too small, the output is safely truncated and NUL-terminated
 * when capacity is nonzero. A NULL buffer is allowed only with zero capacity,
 * which queries the required size.
 *
 * Returns false for invalid arguments, unsupported targets, or generation
 * errors. On false, supplied length outputs are reset to zero.
 */
bool hauvie_router_render_buffer(char *buffer, size_t capacity,
                                 size_t *written, size_t *required,
                                 HauvieTarget target,
                                 const PipelineReport *report);

#endif
