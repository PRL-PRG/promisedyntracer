#ifndef __CONTEXT_H__
#define __CONTEXT_H__

#include "AnalysisDriver.h"
#include "DebugSerializer.h"
#include "State.h"
#include "TraceSerializer.h"
#include <string>

#define tracer_state(context)                                                  \
    ((static_cast<Context *>(context->dyntracer_context))->get_state())

#define tracer_serializer(context)                                             \
    ((static_cast<Context *>(context->dyntracer_context))->get_serializer())

#define debug_serializer(context)                                              \
    ((static_cast<Context *>(context->dyntracer_context))                      \
         ->get_debug_serializer())

#define analysis_driver(context)                                               \
    ((static_cast<Context *>(context->dyntracer_context))                      \
         ->get_analysis_driver())

class Context {
  public:
    Context(std::string trace_filepath, bool truncate, int verbose,
            std::string output_dir, bool enable_analysis);
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

#endif /* __CONTEXT_H__ */
