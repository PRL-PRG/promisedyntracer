#ifndef __TRACE_SERIALIZER_H__
#define __TRACE_SERIALIZER_H__

#include "State.h"
#include "stdlibs.h"
#include "utilities.h"

class TraceSerializer {
  public:
    static const std::string OPCODE_FUNCTION_BEGIN;
    static const std::string OPCODE_FUNCTION_FINISH;
    static const std::string OPCODE_FUNCTION_CONTEXT_JUMP;
    static const std::string OPCODE_PROMISE_CREATE;
    static const std::string OPCODE_PROMISE_BEGIN;
    static const std::string OPCODE_PROMISE_FINISH;
    static const std::string OPCODE_PROMISE_VALUE_LOOKUP;
    static const std::string OPCODE_PROMISE_EXPRESSION_LOOKUP;
    static const std::string OPCODE_PROMISE_ENVIRONMENT_LOOKUP;
    static const std::string OPCODE_PROMISE_VALUE_ASSIGN;
    static const std::string OPCODE_PROMISE_EXPRESSION_ASSIGN;
    static const std::string OPCODE_PROMISE_ENVIRONMENT_ASSIGN;
    static const std::string OPCODE_PROMISE_CONTEXT_JUMP;
    static const std::string OPCODE_ENVIRONMENT_CREATE;
    static const std::string OPCODE_ENVIRONMENT_ASSIGN;
    static const std::string OPCODE_ENVIRONMENT_REMOVE;
    static const std::string OPCODE_ENVIRONMENT_DEFINE;
    static const std::string OPCODE_ENVIRONMENT_LOOKUP;

    TraceSerializer(std::string trace_filepath, bool truncate,
                    bool enable_trace)
        : trace_filepath(trace_filepath), enable_trace_(enable_trace) {
        open_trace(trace_filepath, truncate);
    }

    template <typename T> void serialize(T value) {
        if (enable_trace()) {
            trace << value << RECORD_SEPARATOR << std::endl;
        }
    }

    template <typename T, typename... Args>
    void serialize(T value, Args... args) {
        if (enable_trace()) {
            trace << value << UNIT_SEPARATOR;
            serialize(args...);
        }
    }

    ~TraceSerializer() { close_trace(); }

  private:
    void open_trace(const std::string &trace_filepath, bool truncate) {
        if (!enable_trace())
            return;
        if (file_exists(trace_filepath)) {
            if (truncate)
                remove(trace_filepath.c_str());
            else {
                dyntrace_log_error("trace file '%s' already exists and "
                                   "truncate flag is false",
                                   trace_filepath.c_str());
            }
        }
        trace.open(trace_filepath);
        if (!trace.good()) {
            close_trace();
            dyntrace_log_error(
                "invalid state of stream object associated with %s",
                trace_filepath.c_str());
        }
    }

    void close_trace() {
        if (trace.is_open())
            trace.close();
    }

    bool enable_trace() const { return enable_trace_; }

    std::string trace_filepath;
    std::ofstream trace;
    bool enable_trace_;
};

#endif /* __TRACE_SERIALIZER_H__ */
