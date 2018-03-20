#ifndef __CONTEXT_H__
#define __CONTEXT_H__

#include "SqlSerializer.h"
#include "State.h"
#include <string>
#include "DebugSerializer.h"

#define tracer_state(context)                                                  \
    ((static_cast<Context *>(context->dyntracer_context))->get_state())
#define tracer_serializer(context)                                             \
    ((static_cast<Context *>(context->dyntracer_context))->get_serializer())
#define debug_serializer(context)                                              \
    ((static_cast<Context *>(context->dyntracer_context))->get_debug_serializer())



class Context {
  public:
    Context(std::string database, std::string schema, bool truncate,
            int verbose);
    tracer_state_t &get_state();
    SqlSerializer &get_serializer();
    DebugSerializer &get_debug_serializer();

  private:
    tracer_state_t *state;
    SqlSerializer *serializer;
    DebugSerializer *debugger;
};

#endif /* __CONTEXT_H__ */
