#ifndef __CONTEXT_H__
#define __CONTEXT_H__

#include "AnalysisDriver.h"
#include "AnalysisSwitch.h"
#include "DebugSerializer.h"
#include "State.h"
#include "TraceSerializer.h"
#include <string>

class Context {
  public:
    Context(std::string trace_filepath, bool truncate, bool enable_trace,
            bool verbose, std::string output_dir,
            AnalysisSwitch analysis_switch);
    tracer_state_t &get_state();
    TraceSerializer &get_serializer();
    DebugSerializer &get_debug_serializer();
    AnalysisDriver &get_analysis_driver();

  private:
    tracer_state_t *state_;
    TraceSerializer *serializer_;
    DebugSerializer *debugger_;
    AnalysisDriver *driver_;
};

inline tracer_state_t &tracer_state(dyntracer_t *dyntracer) {
    return (static_cast<Context *>(dyntracer->state))->get_state();
}

inline TraceSerializer &tracer_serializer(dyntracer_t *dyntracer) {
    return (static_cast<Context *>(dyntracer->state))->get_serializer();
}

inline DebugSerializer &debug_serializer(dyntracer_t *dyntracer) {
    return (static_cast<Context *>(dyntracer->state))->get_debug_serializer();
}

inline AnalysisDriver &analysis_driver(dyntracer_t *dyntracer) {
    return (static_cast<Context *>(dyntracer->state))->get_analysis_driver();
}

#endif /* __CONTEXT_H__ */
