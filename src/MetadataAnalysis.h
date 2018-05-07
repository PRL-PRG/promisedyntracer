#ifndef __METADATA_ANALYSIS_H__
#define __METADATA_ANALYSIS_H__

#include "State.h"
#include "utilities.h"
#include "timer.h"
using namespace timing;

class MetadataAnalysis {
  public:
    MetadataAnalysis(const tracer_state_t &tracer_state,
                     const std::string &output_dir);
    void begin(dyntrace_context_t *context);
    void end(dyntrace_context_t *context);

  private:
    void serialize_row(std::ofstream &fout, const std::string &key,
                       const std::string &value);
    std::string output_dir_;
    const tracer_state_t &tracer_state_;
};

#endif /* __METADATA_ANALYSIS_H__ */
