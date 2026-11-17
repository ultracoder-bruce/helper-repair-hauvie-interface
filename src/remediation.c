#include "hauvie_remediation.h"

HauvieRemediation hauvie_remediation_for_result(PipelineResult result) {
    switch (result) {
        case PIPELINE_OK:
            return HAUVIE_REMEDIATION_NONE;
        case PIPELINE_CRITICAL_HAUVIES:
            return HAUVIE_REMEDIATION_REVIEW;
        case PIPELINE_INVALID_ARGUMENT:
        case PIPELINE_INFRASTRUCTURE_FAILURE:
        default:
            return HAUVIE_REMEDIATION_UNSPECIFIED;
    }
}
