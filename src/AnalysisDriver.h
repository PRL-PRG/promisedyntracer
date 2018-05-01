#ifndef __ANALYSIS_DRIVER_H__
#define __ANALYSIS_DRIVER_H__

#include "PromiseTypeAnalysis.h"
#include "State.h"
#include "StrictnessAnalysis.h"

class AnalysisDriver {

  public:
    AnalysisDriver(const tracer_state_t &tracer_state,
                   const std::string &output_dir);
    void promise_created(const prom_basic_info_t &prom_basic_info,
                         const SEXP promise);
    void closure_entry(const closure_info_t &closure_info);
    void closure_exit(const closure_info_t &closure_info);
    void promise_force_entry(const prom_info_t &prom_info);
    void promise_force_exit(const prom_info_t &prom_info, const SEXP promise);
    void gc_promise_unmarked(const prom_id_t &prom_id, const SEXP promise);
    void vector_alloc(const type_gc_info_t &type_gc_info);
    void serialize();

  private:
    StrictnessAnalysis strictness_analysis_;
    PromiseTypeAnalysis promise_type_analysis_;
    std::string output_dir_;
};

#endif /* __ANALYSIS_DRIVER_H__ */
