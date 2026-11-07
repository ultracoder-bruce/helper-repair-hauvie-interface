#include "hauvie_router.h"

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

bool hauvie_router_render(FILE *output, HauvieTarget target,
                          const PipelineReport *report) {
    if (output == NULL || report == NULL) {
        return false;
    }

    if (target == HAUVIE_TARGET_C) {
        return fprintf(output,
            "/* Hauvie route: C; diagnostic result: %s; cycles: %u */\n"
            "#include <stdio.h>\n"
            "int main(void) {\n"
            "    puts(\"Hauvie C template ready\");\n"
            "    return %d;\n"
            "}\n",
            result_name(report->result), report->cycles_completed,
            report->result == PIPELINE_OK ? 0 : 1) >= 0;
    }
    if (target == HAUVIE_TARGET_JAVA) {
        return fprintf(output,
            "// Hauvie route: Java; diagnostic result: %s; cycles: %u\n"
            "public final class HauvieOutput {\n"
            "    public static void main(String[] args) {\n"
            "        System.out.println(\"Hauvie Java template ready\");\n"
            "    }\n"
            "}\n",
            result_name(report->result), report->cycles_completed) >= 0;
    }
    return false;
}
