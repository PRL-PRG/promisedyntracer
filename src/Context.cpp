#include "Context.h"

Context::Context(std::string output_dir, std::string database,
                 std::string schema, bool truncate, int verbose)
    : state_(new tracer_state_t()),
      driver_(new AnalysisDriver(*state_, output_dir)),
      serializer_(new SqlSerializer(database, schema, truncate, verbose)),
      debugger_(new DebugSerializer(verbose)) {}

tracer_state_t &Context::get_state() { return *state_; }
SqlSerializer &Context::get_serializer() { return *serializer_; }
DebugSerializer &Context::get_debug_serializer() {
    if (debugger_->needsState())
        debugger_->setState(&get_state());
    return *debugger_;
}

AnalysisDriver &Context::get_analysis_driver() { return *driver_; }
