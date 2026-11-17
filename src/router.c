#include "hauvie_router.h"

#include <stdio.h>

static const char *result_name(PipelineResult result) {
    switch (result) {
        case PIPELINE_OK:
            return "PIPELINE_OK";
        case PIPELINE_CRITICAL_HAUVIES:
            return "PIPELINE_CRITICAL_HAUVIES";
        case PIPELINE_INVALID_ARGUMENT:
            return "PIPELINE_INVALID_ARGUMENT";
        case PIPELINE_INFRASTRUCTURE_FAILURE:
            return "PIPELINE_INFRASTRUCTURE_FAILURE";
        default:
            return "PIPELINE_UNKNOWN";
    }
}

bool hauvie_router_render_buffer(char *buffer, size_t capacity,
                                 size_t *written, size_t *required,
                                 HauvieTarget target,
                                 const PipelineReport *report) {
    int formatted;

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

    if (target == HAUVIE_TARGET_C) {
        formatted = snprintf(buffer, capacity,
            "/* Hauvie route: C; diagnostic result: %s; cycles: %u */\n"
            "#include <stdio.h>\n"
            "int main(void) {\n"
            "    puts(\"Hauvie C template ready\");\n"
            "    return %d;\n"
            "}\n",
            result_name(report->result), report->cycles_completed,
            report->result == PIPELINE_OK ? 0 : 1);
    } else if (target == HAUVIE_TARGET_JAVA) {
        formatted = snprintf(buffer, capacity,
            "// Hauvie route: Java; diagnostic result: %s; cycles: %u\n"
            "public final class HauvieOutput {\n"
            "    public static void main(String[] args) {\n"
            "        System.out.println(\"Hauvie Java template ready\");\n"
            "    }\n"
            "}\n",
            result_name(report->result), report->cycles_completed);
    } else {
        return false;
    }

    if (formatted < 0) {
        if (capacity > 0) {
            buffer[0] = '\0';
        }
        return false;
    }

    *required = (size_t)formatted;
    *written = capacity == 0 ? 0 :
        (*required < capacity ? *required : capacity - 1);
    return true;
}

bool hauvie_router_render(FILE *output, HauvieTarget target,
                          const PipelineReport *report) {
    char buffer[512];
    size_t written;
    size_t required;

    if (output == NULL ||
        !hauvie_router_render_buffer(buffer, sizeof(buffer), &written,
                                     &required, target, report) ||
        written != required) {
        return false;
    }
    return fwrite(buffer, 1, written, output) == written;
}
