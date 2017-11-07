#include "Context.h"

Context::Context(std::string database, std::string schema, bool verbose)
    : state(new tracer_state_t()),
      serializer(new SqlSerializer(database, schema, verbose)) {}

tracer_state_t &Context::get_state() { return *state; }
SqlSerializer &Context::get_serializer() { return *serializer; }
