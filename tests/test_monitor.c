#include "hauvie_monitor.h"

#include <assert.h>
#include <stdio.h>
#include <string.h>

typedef struct {
    unsigned int cycle_events;
    unsigned int diagnostic_events;
    char diagnostic_key[HAUVIE_CODING_MAX_KEY_LENGTH + 1u];
    char coding_code[HAUVIE_CODING_MAX_CODE_LENGTH + 1u];
    PipelineResult final_result;
} MonitorCapture;

static void capture_event(void *context, const HauvieMonitorEvent *event) {
    MonitorCapture *capture = context;
    if (event->type == HAUVIE_MONITOR_CYCLE_COMPLETED) {
        ++capture->cycle_events;
        assert(event->diagnostic_key == NULL);
        assert(event->coding_code == NULL);
        assert(event->report.cycles_completed == capture->cycle_events);
    } else {
        ++capture->diagnostic_events;
        (void)snprintf(capture->diagnostic_key,
                       sizeof(capture->diagnostic_key), "%s",
                       event->diagnostic_key);
        if (event->coding_code != NULL) {
            (void)snprintf(capture->coding_code, sizeof(capture->coding_code),
                           "%s", event->coding_code);
        }
        capture->final_result = event->report.result;
    }
}

static HauvieCodingMap load_map(void) {
    static const char json[] =
        "{\"schema_version\":1,\"coding_system\":\"ICD-10-CM\","
        "\"mappings\":["
        "{\"diagnostic_key\":\"monitor.pipeline_ok\",\"code\":\"A00\"},"
        "{\"diagnostic_key\":\"monitor.critical_hauvies\",\"code\":\"B00\"}]}";
    HauvieCodingMap map;
    assert(hauvie_coding_map_load_json(json, &map));
    return map;
}

static void test_loop_progress_and_configured_code_are_reported(void) {
    HauvieCodingMap map = load_map();
    MonitorCapture capture = {0};
    HauviePatientMonitor monitor = {
        .callback = capture_event,
        .context = &capture,
        .coding_map = &map
    };
    HauviesPatient patient = {
        .individual_id = "private-patient-id",
        .hauvies_severity = 0.2f,
        .raw_binary_volume = 4096
    };
    ExtruderContext context;
    PipelineReport report;

    assert(hauvies_helper_run_monitored(&context, &patient, &report, &monitor) ==
           PIPELINE_OK);
    assert(capture.cycle_events == PIPELINE_CYCLES);
    assert(capture.diagnostic_events == 1u);
    assert(strcmp(capture.diagnostic_key, "monitor.pipeline_ok") == 0);
    assert(strcmp(capture.coding_code, "A00") == 0);
    assert(capture.final_result == PIPELINE_OK);
    assert(report.cycles_completed == PIPELINE_CYCLES);
}

static void test_critical_event_and_unmapped_case(void) {
    HauvieCodingMap map = load_map();
    MonitorCapture capture = {0};
    HauviePatientMonitor monitor = {
        .callback = capture_event,
        .context = &capture,
        .coding_map = &map
    };
    HauviesPatient patient = {
        .individual_id = "another-private-id",
        .hauvies_severity = 0.9f,
        .raw_binary_volume = 4096
    };
    ExtruderContext context;
    PipelineReport report;

    assert(hauvies_helper_run_monitored(&context, &patient, &report, &monitor) ==
           PIPELINE_CRITICAL_HAUVIES);
    assert(capture.cycle_events == 0u);
    assert(capture.diagnostic_events == 1u);
    assert(strcmp(capture.diagnostic_key, "monitor.critical_hauvies") == 0);
    assert(strcmp(capture.coding_code, "B00") == 0);
    assert(capture.final_result == PIPELINE_CRITICAL_HAUVIES);

    memset(&capture, 0, sizeof(capture));
    patient.hauvies_severity = 0.4f;
    assert(hauvies_helper_run_monitored(&context, &patient, &report, &monitor) ==
           PIPELINE_OK);
    assert(strcmp(capture.coding_code, "A00") == 0);
}

static void test_invalid_patient_notifies_without_code_when_unmapped(void) {
    MonitorCapture capture = {0};
    HauviePatientMonitor monitor = {
        .callback = capture_event,
        .context = &capture,
        .coding_map = NULL
    };
    ExtruderContext context;
    PipelineReport report;

    assert(hauvies_helper_run_monitored(&context, NULL, &report, &monitor) ==
           PIPELINE_INVALID_ARGUMENT);
    assert(capture.diagnostic_events == 1u);
    assert(capture.final_result == PIPELINE_INVALID_ARGUMENT);
    assert(capture.coding_code[0] == '\0');
}

int main(void) {
    test_loop_progress_and_configured_code_are_reported();
    test_critical_event_and_unmapped_case();
    test_invalid_patient_notifies_without_code_when_unmapped();
    puts("Patient monitor callback integration tests passed.");
    return 0;
}
