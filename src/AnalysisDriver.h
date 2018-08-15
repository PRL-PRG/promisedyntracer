#ifndef __ANALYSIS_DRIVER_H__
#define __ANALYSIS_DRIVER_H__

#include "AnalysisSwitch.h"
#include "FunctionAnalysis.h"
#include "MetadataAnalysis.h"
#include "ObjectCountSizeAnalysis.h"
#include "PromiseEvaluationAnalysis.h"
#include "PromiseTypeAnalysis.h"
#include "SideEffectAnalysis.h"
#include "State.h"
#include "StrictnessAnalysis.h"

class AnalysisDriver {

  public:
    AnalysisDriver(tracer_state_t &tracer_state, const std::string &output_dir,
                   bool truncate, bool binary, int compression_level,
                   const AnalysisSwitch analysis_switch);

    void begin(dyntracer_t *dyntracer);
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
    void promise_environment_lookup(const prom_info_t &info,
                                    const SEXP promise);
    void promise_expression_lookup(const prom_info_t &info, const SEXP promise);
    void promise_value_lookup(const prom_info_t &info, const SEXP promise);
    void promise_environment_set(const prom_info_t &info, const SEXP promise);
    void promise_expression_set(const prom_info_t &info, const SEXP promise);
    void promise_value_set(const prom_info_t &info, const SEXP promise);

    void gc_promise_unmarked(const prom_id_t prom_id, const SEXP promise);
    void vector_alloc(const type_gc_info_t &type_gc_info);
    void environment_define_var(const SEXP symbol, const SEXP value,
                                const SEXP rho);
    void environment_assign_var(const SEXP symbol, const SEXP value,
                                const SEXP rho);
    void environment_lookup_var(const SEXP symbol, const SEXP value,
                                const SEXP rho);
    void environment_remove_var(const SEXP symbol, const SEXP rho);
    void context_jump(const unwind_info_t &info);
    void end(dyntracer_t *dyntracer);

    inline bool analyze_metadata() const;
    inline bool analyze_object_count_size() const;
    inline bool analyze_promise_types() const;
    inline bool analyze_promise_slot_mutations() const;
    inline bool analyze_promise_evaluations() const;
    inline bool analyze_functions() const;
    inline bool analyze_strictness() const;
    inline bool analyze_side_effects() const;
    inline bool map_promises() const;

  private:
    PromiseMapper promise_mapper_;
    FunctionAnalysis function_analysis_;
    StrictnessAnalysis strictness_analysis_;
    PromiseTypeAnalysis promise_type_analysis_;
    PromiseEvaluationAnalysis promise_evaluation_analysis_;
    SideEffectAnalysis side_effect_analysis_;
    ObjectCountSizeAnalysis object_count_size_analysis_;
    MetadataAnalysis metadata_analysis_;
    AnalysisSwitch analysis_switch_;
};

#endif /* __ANALYSIS_DRIVER_H__ */
