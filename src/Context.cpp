#include "Context.h"

Context::Context(std::string trace_filepath, bool truncate, int verbose,
                 std::string output_dir, bool enable_analysis)
    : state_(new tracer_state_t()),
      serializer_(new TraceSerializer(trace_filepath, truncate)),
      driver_(new AnalysisDriver(*state_, output_dir, enable_analysis)),
      debugger_(new DebugSerializer(verbose)) {}

tracer_state_t &Context::get_state() { return *state_; }
TraceSerializer &Context::get_serializer() { return *serializer_; }
DebugSerializer &Context::get_debug_serializer() {
    if (debugger_->needsState())
        debugger_->setState(&get_state());
    return *debugger_;
}

AnalysisDriver &Context::get_analysis_driver() { return *driver_; }
