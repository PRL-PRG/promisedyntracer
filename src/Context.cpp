#include "Context.h"

Context::Context(std::string database, std::string schema, bool truncate,
                 int verbose)
    : state(new tracer_state_t()),
      serializer(new SqlSerializer(database, schema, truncate, verbose)),
      debugger(new DebugSerializer(verbose)) {}

tracer_state_t &Context::get_state() { return *state; }
SqlSerializer &Context::get_serializer() { return *serializer; }
DebugSerializer &Context::get_debug_serializer() {
    if(debugger->needsState())
        debugger->setState(&get_state());
    return *debugger;
}
