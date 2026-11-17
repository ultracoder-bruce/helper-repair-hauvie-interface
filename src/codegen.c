#include "hauvie_codegen.h"

#include <stdarg.h>
#include <stdio.h>

/* Each backend is a visitor over the generated document's three parts. */
typedef struct {
    bool (*visit_begin)(const HauvieCodegenSink *sink,
                        const PipelineReport *report);
    bool (*visit_report)(const HauvieCodegenSink *sink,
                         const PipelineReport *report);
    bool (*visit_end)(const HauvieCodegenSink *sink,
                      const PipelineReport *report);
} HauvieCodegenVisitor;

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

static bool emit(const HauvieCodegenSink *sink, const char *text, size_t length) {
    return sink->write(sink->context, text, length);
}

static bool emit_format(const HauvieCodegenSink *sink, const char *format, ...) {
    char text[160];
    va_list args;
    va_start(args, format);
    int length = vsnprintf(text, sizeof(text), format, args);
    va_end(args);
    if (length < 0 || (size_t)length >= sizeof(text)) {
        return false;
    }
    return emit(sink, text, (size_t)length);
}

static bool c_visit_begin(const HauvieCodegenSink *sink,
                          const PipelineReport *report) {
    return emit_format(sink,
        "/* Hauvie route: C; diagnostic result: %s; cycles: %u */\n"
        "#include <stdio.h>\n"
        "int main(void) {\n",
        result_name(report->result), report->cycles_completed);
}

static bool c_visit_report(const HauvieCodegenSink *sink,
                           const PipelineReport *report) {
    return emit_format(sink,
        "    puts(\"Hauvie C template ready\");\n"
        "    return %d;\n",
        report->result == PIPELINE_OK ? 0 : 1);
}

static bool c_visit_end(const HauvieCodegenSink *sink,
                        const PipelineReport *report) {
    (void)report;
    return emit(sink, "}\n", sizeof("}\n") - 1);
}

static bool java_visit_begin(const HauvieCodegenSink *sink,
                             const PipelineReport *report) {
    return emit_format(sink,
        "// Hauvie route: Java; diagnostic result: %s; cycles: %u\n"
        "public final class HauvieOutput {\n"
        "    public static void main(String[] args) {\n",
        result_name(report->result), report->cycles_completed);
}

static bool java_visit_report(const HauvieCodegenSink *sink,
                              const PipelineReport *report) {
    (void)report;
    return emit(sink, "        System.out.println(\"Hauvie Java template ready\");\n",
                sizeof("        System.out.println(\"Hauvie Java template ready\");\n") - 1);
}

static bool java_visit_end(const HauvieCodegenSink *sink,
                           const PipelineReport *report) {
    (void)report;
    return emit(sink, "    }\n}\n", sizeof("    }\n}\n") - 1);
}

static const HauvieCodegenVisitor c_visitor = {
    .visit_begin = c_visit_begin,
    .visit_report = c_visit_report,
    .visit_end = c_visit_end
};

static const HauvieCodegenVisitor java_visitor = {
    .visit_begin = java_visit_begin,
    .visit_report = java_visit_report,
    .visit_end = java_visit_end
};

bool hauvie_codegen_generate(const HauvieCodegenSink *sink,
                             HauvieTarget target,
                             const PipelineReport *report) {
    const HauvieCodegenVisitor *visitor;

    if (sink == NULL || sink->write == NULL || report == NULL) {
        return false;
    }
    if (target == HAUVIE_TARGET_C) {
        visitor = &c_visitor;
    } else if (target == HAUVIE_TARGET_JAVA) {
        visitor = &java_visitor;
    } else {
        return false;
    }

    return visitor->visit_begin(sink, report) &&
           visitor->visit_report(sink, report) &&
           visitor->visit_end(sink, report);
}
