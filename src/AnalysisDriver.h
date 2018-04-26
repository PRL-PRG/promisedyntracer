#ifndef __ANALYSIS_DRIVER_H__
#define __ANALYSIS_DRIVER_H__

#include "State.h"
#include "StrictnessAnalysis.h"

class AnalysisDriver {

  public:
    AnalysisDriver(const tracer_state_t &tracer_state,
                   const std::string &output_dir);
    void closure_entry(const closure_info_t &closure_info);
    void closure_exit(const closure_info_t &closure_info);
    void promise_entry(const prom_info_t &prom_info);
    void promise_exit(const prom_info_t &prom_info);
    void gc_promise_unmarked(const prom_id_t prom_id);
    void serialize();

  private:
    StrictnessAnalysis strictness_analysis_;
    std::string output_dir_;
};

#endif /* __ANALYSIS_DRIVER_H__ */
