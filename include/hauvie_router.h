#ifndef HAUVIE_ROUTER_H
#define HAUVIE_ROUTER_H

#include "hauvie.h"
#include "hauvie_token.h"

#include <stddef.h>
#include <stdio.h>

/* Render a safe, fixed C or Java output template for the parsed target. */
bool hauvie_router_render(FILE *output, HauvieTarget target,
                          const PipelineReport *report);

/*
 * Render into caller-owned storage. `required` receives the number of output
 * bytes excluding the NUL terminator. `written` receives the number of output
 * bytes actually stored, also excluding the terminator. If capacity is too
 * small, the output is safely truncated and NUL-terminated (when capacity is
 * nonzero), and the function still returns true. A NULL buffer is allowed only
 * when capacity is zero, which can be used to query the required size.
 *
 * Returns false for invalid arguments or an unsupported target. On false,
 * supplied length outputs are reset to zero.
 */
bool hauvie_router_render_buffer(char *buffer, size_t capacity,
                                size_t *written, size_t *required,
                                HauvieTarget target,
                                const PipelineReport *report);

#endif
