#include "hauvie_router.h"
#include "hauvie_codegen.h"

#include <stdio.h>

typedef struct {
    char *buffer;
    size_t capacity;
    size_t written;
    size_t required;
} BufferSink;

static bool file_write(void *context, const char *data, size_t length) {
    FILE *output = context;
    return fwrite(data, 1, length, output) == length;
}

static bool buffer_write(void *context, const char *data, size_t length) {
    BufferSink *sink = context;
    if (length > (size_t)-1 - sink->required) {
        return false;
    }
    sink->required += length;

    size_t available = sink->capacity == 0 ? 0 : sink->capacity - 1 - sink->written;
    size_t copy_length = length < available ? length : available;
    if (copy_length > 0) {
        for (size_t i = 0; i < copy_length; ++i) {
            sink->buffer[sink->written + i] = data[i];
        }
        sink->written += copy_length;
        sink->buffer[sink->written] = '\0';
    }
    return true;
}

bool hauvie_router_render_buffer(char *buffer, size_t capacity,
                                 size_t *written, size_t *required,
                                 HauvieTarget target,
                                 const PipelineReport *report) {
    BufferSink state = {
        .buffer = buffer,
        .capacity = capacity,
        .written = 0,
        .required = 0
    };
    HauvieCodegenSink sink = {
        .write = buffer_write,
        .context = &state
    };

    if (written != NULL) {
        *written = 0;
    }
    if (required != NULL) {
        *required = 0;
    }
    if (report == NULL || written == NULL || required == NULL ||
        (capacity > 0 && buffer == NULL)) {
        return false;
    }
    if (capacity > 0) {
        buffer[0] = '\0';
    }
    if (!hauvie_codegen_generate(&sink, target, report)) {
        if (capacity > 0) {
            buffer[0] = '\0';
        }
        return false;
    }

    *written = state.written;
    *required = state.required;
    return true;
}

bool hauvie_router_render(FILE *output, HauvieTarget target,
                          const PipelineReport *report) {
    HauvieCodegenSink sink = {
        .write = file_write,
        .context = output
    };
    return output != NULL && hauvie_codegen_generate(&sink, target, report);
}
