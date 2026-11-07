#ifndef HAUVIE_ROUTER_H
#define HAUVIE_ROUTER_H

#include "hauvie.h"
#include "hauvie_token.h"

#include <stdio.h>

/* Render a safe, fixed C or Java output template for the parsed target. */
bool hauvie_router_render(FILE *output, HauvieTarget target,
                          const PipelineReport *report);

#endif
