#ifndef __METADATA_ANALYSIS_H__
#define __METADATA_ANALYSIS_H__

#include "State.h"
#include "Timer.h"
#include "utilities.h"

class MetadataAnalysis {
  public:
    MetadataAnalysis(const tracer_state_t &tracer_state,
                     const std::string &output_dir);
    void end(dyntracer_t *dyntracer);

  private:
    void serialize_row(std::ofstream &fout,
                       std::string key,
                       std::string value);
    std::string output_dir_;
    const tracer_state_t &tracer_state_;
};

#endif /* __METADATA_ANALYSIS_H__ */
