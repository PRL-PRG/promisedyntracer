#include "TraceSerializer.h"

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
