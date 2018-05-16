#ifndef __TRACE_SERIALIZER_H__
#define __TRACE_SERIALIZER_H__

#include "State.h"
#include "stdlibs.h"
#include "utilities.h"

class TraceSerializer {
  public:
    TraceSerializer(std::string trace_filepath, bool truncate,
                    bool enable_trace);
    ~TraceSerializer();
    void serialize_trace(const std::string &opcode, const int id_1);
    void serialize_trace(const std::string &opcode, const int id_1,
                         const std::string id_2);
    void serialize_trace(const std::string &opcode, const int id_1,
                         const int id_2);
    void serialize_trace(const std::string &opcode, const int id_1,
                         const int id_2, const std::string &symbol,
                         const std::string sexptype);
    void serialize_trace(const std::string &opcode, const fn_id_t &id_1,
                         const int id_2, const int id_3);

  private:
    void open_trace(const std::string &trace_filepath, bool truncate);
    void close_trace();
    inline bool enable_trace() const;
    std::string trace_filepath;
    std::ofstream trace;
    bool enable_trace_;
};

#endif /* __TRACE_SERIALIZER_H__ */
