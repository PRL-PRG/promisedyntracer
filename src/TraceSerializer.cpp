#include "TraceSerializer.h"

const std::string TraceSerializer::OPCODE_CLOSURE_BEGIN = "clb";
const std::string TraceSerializer::OPCODE_CLOSURE_FINISH = "clf";
const std::string TraceSerializer::OPCODE_BUILTIN_BEGIN = "bub";
const std::string TraceSerializer::OPCODE_BUILTIN_FINISH = "buf";
const std::string TraceSerializer::OPCODE_SPECIAL_BEGIN = "spb";
const std::string TraceSerializer::OPCODE_SPECIAL_FINISH = "spf";
const std::string TraceSerializer::OPCODE_FUNCTION_CONTEXT_JUMP = "fnj";
const std::string TraceSerializer::OPCODE_PROMISE_CREATE = "prc";
const std::string TraceSerializer::OPCODE_PROMISE_BEGIN = "prb";
const std::string TraceSerializer::OPCODE_PROMISE_FINISH = "prf";
const std::string TraceSerializer::OPCODE_PROMISE_VALUE = "prv";
const std::string TraceSerializer::OPCODE_PROMISE_CONTEXT_JUMP = "prj";
const std::string TraceSerializer::OPCODE_PROMISE_EXPRESSION = "pre";
const std::string TraceSerializer::OPCODE_PROMISE_ENVIRONMENT = "pen";
const std::string TraceSerializer::OPCODE_ENVIRONMENT_CREATE = "enc";
const std::string TraceSerializer::OPCODE_ENVIRONMENT_ASSIGN = "ena";
const std::string TraceSerializer::OPCODE_ENVIRONMENT_REMOVE = "enr";
const std::string TraceSerializer::OPCODE_ENVIRONMENT_DEFINE = "end";
const std::string TraceSerializer::OPCODE_ENVIRONMENT_LOOKUP = "enl";

TraceSerializer::TraceSerializer(std::string trace_filepath, bool truncate,
                                 bool enable_trace)
    : trace_filepath(trace_filepath), enable_trace_(enable_trace) {
    open_trace(trace_filepath, truncate);
}

void TraceSerializer::open_trace(const std::string &trace_filepath,
                                 bool truncate) {
    if (!enable_trace())
        return;
    if (file_exists(trace_filepath)) {
        if (truncate)
            remove(trace_filepath.c_str());
        else {
            dyntrace_log_error(
                "trace file '%s' already exists and truncate flag is false",
                trace_filepath.c_str());
        }
    }
    trace.open(trace_filepath);
    if (!trace.good()) {
        close_trace();
        dyntrace_log_error("invalid state of stream object associated with %s",
                           trace_filepath.c_str());
    }
}

TraceSerializer::~TraceSerializer() { close_trace(); }

void TraceSerializer::close_trace() {
    if (trace.is_open())
        trace.close();
}

void TraceSerializer::serialize_trace(const std::string &opcode,
                                      const int id_1) {
    if (enable_trace())
        trace << opcode << " " << id_1 << std::endl;
}

void TraceSerializer::serialize_trace(const std::string &opcode, const int id_1,
                                      const std::string id_2) {
    if (enable_trace())
        trace << opcode << " " << id_1 << " " << id_2 << std::endl;
}

void TraceSerializer::serialize_trace(const std::string &opcode, const int id_1,
                                      const int id_2) {
    if (enable_trace())
        trace << opcode << " " << id_1 << " " << id_2 << std::endl;
}

void TraceSerializer::serialize_trace(const std::string &opcode, const int id_1,
                                      const int id_2, const std::string &symbol,
                                      const std::string sexptype) {
    if (enable_trace())
        trace << opcode << " " << id_1 << " " << id_2 << " " << symbol << " "
              << sexptype << std::endl;
}

void TraceSerializer::serialize_trace(const std::string &opcode,
                                      const fn_id_t &id_1, const int id_2,
                                      const int id_3) {
    if (enable_trace())
        trace << opcode << " " << id_1 << " " << id_2 << " " << id_3
              << std::endl;
}

inline bool TraceSerializer::enable_trace() const { return enable_trace_; }
