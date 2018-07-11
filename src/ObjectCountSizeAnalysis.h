#ifndef __OBJECT_COUNT_SIZE_ANALYSIS_H__
#define __OBJECT_COUNT_SIZE_ANALYSIS_H__

#include "State.h"
#include <algorithm>
#include <tuple>
#include <unordered_map>
#include <vector>
#include "utilities.h"

class ObjectCountSizeAnalysis {
  public:
    ObjectCountSizeAnalysis(const tracer_state_t &tracer_state,
                            const std::string &output_dir);
    void vector_alloc(const type_gc_info_t &info);
    void end(dyntracer_t *dyntracer);

  private:
    void serialize();
    std::vector<int> object_counts_;
    std::vector<unsigned long long int> object_sizes_;
    std::string output_dir_;
    const tracer_state_t &tracer_state_;
};

#endif /* __STRICTNESS_ANALYSIS_H__ */
