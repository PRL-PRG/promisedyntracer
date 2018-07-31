#include "Context.h"

Context::Context(std::string trace_filepath, bool truncate, bool enable_trace,
                 bool verbose, std::string output_dir,
                 AnalysisSwitch analysis_switch)
    : state_(new tracer_state_t()),
      serializer_(new TraceSerializer(trace_filepath, truncate, enable_trace)),
      driver_(new AnalysisDriver(*state_, output_dir, analysis_switch)),
      debugger_(new DebugSerializer(verbose)), output_dir_{output_dir} {}

tracer_state_t &Context::get_state() { return *state_; }
TraceSerializer &Context::get_serializer() { return *serializer_; }
DebugSerializer &Context::get_debug_serializer() {
    if (debugger_->needsState())
        debugger_->setState(&get_state());
    return *debugger_;
}

AnalysisDriver &Context::get_analysis_driver() { return *driver_; }

const std::string &Context::get_output_dir() const { return output_dir_; }

Context::~Context() {

    delete debugger_;
    delete driver_;
    delete serializer_;
    /* delete state in the end as everything else
       can store reference to the state */
    delete state_;
}
