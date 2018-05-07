#ifndef __ANALYSIS_DRIVER_H__
#define __ANALYSIS_DRIVER_H__

#include "FunctionReturnTypeAnalysis.h"
#include "ObjectCountSizeAnalysis.h"
#include "PromiseEvaluationDistanceAnalysis.h"
#include "PromiseTypeAnalysis.h"
#include "SideEffectAnalysis.h"
#include "State.h"
#include "StrictnessAnalysis.h"
#include "MetadataAnalysis.h"

class AnalysisDriver {

  public:
    AnalysisDriver(const tracer_state_t &tracer_state,
                   const std::string &output_dir, bool is_enabled);

    void begin(dyntrace_context_t *context);
    void closure_entry(const closure_info_t &closure_info);
    void closure_exit(const closure_info_t &closure_info);
    void special_entry(const builtin_info_t &special_info);
    void special_exit(const builtin_info_t &special_info);
    void builtin_entry(const builtin_info_t &builtin_info);
    void builtin_exit(const builtin_info_t &builtin_info);

    void promise_created(const prom_basic_info_t &prom_basic_info,
                         const SEXP promise);
    void promise_force_entry(const prom_info_t &prom_info, const SEXP promise);
    void promise_force_exit(const prom_info_t &prom_info, const SEXP promise);

    void promise_environment_lookup(const prom_info_t &info, const SEXP promise,
                                    int in_force);
    void promise_expression_lookup(const prom_info_t &info, const SEXP promise,
                                   int in_force);
    void promise_value_lookup(const prom_info_t &info, const SEXP promise,
                              int in_force);
    void promise_environment_set(const prom_info_t &info, const SEXP promise,
                                 int in_force);
    void promise_expression_set(const prom_info_t &info, const SEXP promise,
                                int in_force);
    void promise_value_set(const prom_info_t &info, const SEXP promise,
                           int in_force);

    void gc_promise_unmarked(const prom_id_t prom_id, const SEXP promise);
    void vector_alloc(const type_gc_info_t &type_gc_info);
    void environment_define_var(const SEXP symbol, const SEXP value,
                                const SEXP rho);
    void environment_assign_var(const SEXP symbol, const SEXP value,
                                const SEXP rho);
    void environment_lookup_var(const SEXP symbol, const SEXP value,
                                const SEXP rho);
    void environment_remove_var(const SEXP symbol, const SEXP rho);
    void end(dyntrace_context_t *context);

  private:
    StrictnessAnalysis strictness_analysis_;
    PromiseTypeAnalysis promise_type_analysis_;
    SideEffectAnalysis side_effect_analysis_;
    PromiseEvaluationDistanceAnalysis promise_evaluation_distance_analysis_;
    ObjectCountSizeAnalysis object_count_size_analysis_;
    FunctionReturnTypeAnalysis function_return_type_analysis_;
    MetadataAnalysis metadata_analysis_;
    bool is_enabled;
};

#endif /* __ANALYSIS_DRIVER_H__ */
