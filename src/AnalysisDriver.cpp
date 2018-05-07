#include "AnalysisDriver.h"

AnalysisDriver::AnalysisDriver(const tracer_state_t &tracer_state,
                               const std::string &output_dir, bool is_enabled)
    : strictness_analysis_(StrictnessAnalysis(tracer_state, output_dir)),
      object_count_size_analysis_(
          ObjectCountSizeAnalysis(tracer_state, output_dir)),
      promise_type_analysis_(PromiseTypeAnalysis(tracer_state, output_dir)),
      promise_evaluation_distance_analysis_(
          PromiseEvaluationDistanceAnalysis(tracer_state, output_dir)),
      side_effect_analysis_(SideEffectAnalysis(tracer_state, output_dir)),
      function_return_type_analysis_(
          FunctionReturnTypeAnalysis(tracer_state, output_dir)),
      metadata_analysis_(MetadataAnalysis(tracer_state, output_dir)),
      is_enabled(is_enabled) {}

void AnalysisDriver::begin(dyntrace_context_t *context) {
    if (is_enabled) {
        metadata_analysis_.begin(context);
    }
}

void AnalysisDriver::promise_created(const prom_basic_info_t &prom_basic_info,
                                     const SEXP promise) {
    if (is_enabled) {
        promise_type_analysis_.promise_created(prom_basic_info, promise);
    }
    // promise_evaluation_distance_analysis_.promise_created(prom_basic_info,
    //                                                          promise);
}

void AnalysisDriver::closure_entry(const closure_info_t &closure_info) {
    if (is_enabled) {
        strictness_analysis_.closure_entry(closure_info);
        promise_type_analysis_.closure_entry(closure_info);
    }
    // promise_evaluation_distance_analysis_.closure_entry(closure_info);
}

void AnalysisDriver::special_entry(const builtin_info_t &special_info) {
    // promise_evaluation_distance_analysis_.special_entry(special_info);
}

void AnalysisDriver::builtin_entry(const builtin_info_t &builtin_info) {
    // promise_evaluation_distance_analysis_.builtin_entry(builtin_info);
}

void AnalysisDriver::closure_exit(const closure_info_t &closure_info) {
    if (is_enabled) {
        strictness_analysis_.closure_exit(closure_info);
        function_return_type_analysis_.closure_exit(closure_info);
    }
}

void AnalysisDriver::builtin_exit(const builtin_info_t &builtin_info) {
    if (is_enabled) {
        function_return_type_analysis_.builtin_exit(builtin_info);
    }
}

void AnalysisDriver::special_exit(const builtin_info_t &special_info) {
    if (is_enabled) {
        function_return_type_analysis_.special_exit(special_info);
    }
}

void AnalysisDriver::promise_force_entry(const prom_info_t &prom_info,
                                         const SEXP promise) {
    if (is_enabled) {
        strictness_analysis_.promise_force_entry(prom_info, promise);
    }
    // promise_evaluation_distance_analysis_.promise_force_entry(prom_info,
    //                                                              promise);
}

void AnalysisDriver::promise_force_exit(const prom_info_t &prom_info,
                                        const SEXP promise) {
    if (is_enabled) {
        promise_type_analysis_.promise_force_exit(prom_info, promise);
    }
}

void AnalysisDriver::gc_promise_unmarked(const prom_id_t prom_id,
                                         const SEXP promise) {
    if (is_enabled) {
        strictness_analysis_.gc_promise_unmarked(prom_id, promise);
        promise_type_analysis_.gc_promise_unmarked(prom_id, promise);
    }
    // promise_evaluation_distance_analysis_.gc_promise_unmarked(prom_id,
    // promise);
}

void AnalysisDriver::promise_environment_lookup(const prom_info_t &info,
                                                const SEXP promise,
                                                int in_force) {
    if (is_enabled) {
        strictness_analysis_.promise_environment_lookup(info, promise,
                                                        in_force);
    }
}

void AnalysisDriver::promise_expression_lookup(const prom_info_t &info,
                                               const SEXP promise,
                                               int in_force) {
    if (is_enabled) {
        strictness_analysis_.promise_expression_lookup(info, promise, in_force);
    }
}

void AnalysisDriver::promise_value_lookup(const prom_info_t &info,
                                          const SEXP promise, int in_force) {
    if (is_enabled) {
        strictness_analysis_.promise_value_lookup(info, promise, in_force);
    }
}

void AnalysisDriver::promise_environment_set(const prom_info_t &info,
                                             const SEXP promise, int in_force) {
    if (is_enabled) {
        strictness_analysis_.promise_environment_set(info, promise, in_force);
    }
}

void AnalysisDriver::promise_expression_set(const prom_info_t &info,
                                            const SEXP promise, int in_force) {
    if (is_enabled) {
        strictness_analysis_.promise_expression_set(info, promise, in_force);
    }
}

void AnalysisDriver::promise_value_set(const prom_info_t &info,
                                       const SEXP promise, int in_force) {
    if (is_enabled) {
        strictness_analysis_.promise_value_set(info, promise, in_force);
    }
}

void AnalysisDriver::vector_alloc(const type_gc_info_t &type_gc_info) {
    if (is_enabled) {
        object_count_size_analysis_.vector_alloc(type_gc_info);
    }
}

void AnalysisDriver::environment_define_var(const SEXP symbol, const SEXP value,
                                            const SEXP rho) {
    if (is_enabled) {
        side_effect_analysis_.environment_define_var(symbol, value, rho);
    }
}

void AnalysisDriver::environment_assign_var(const SEXP symbol, const SEXP value,
                                            const SEXP rho) {
    if (is_enabled) {
        side_effect_analysis_.environment_assign_var(symbol, value, rho);
    }
}

void AnalysisDriver::environment_lookup_var(const SEXP symbol, const SEXP value,
                                            const SEXP rho) {
    if (is_enabled) {
        side_effect_analysis_.environment_lookup_var(symbol, value, rho);
    }
}

void AnalysisDriver::environment_remove_var(const SEXP symbol, const SEXP rho) {
    if (is_enabled) {
        side_effect_analysis_.environment_remove_var(symbol, rho);
    }
}

void AnalysisDriver::end(dyntrace_context_t *context) {
    if (is_enabled) {
        strictness_analysis_.end(context);
        object_count_size_analysis_.end(context);
        promise_type_analysis_.end(context);
        side_effect_analysis_.end(context);
        function_return_type_analysis_.end(context);
        metadata_analysis_.end(context);
    }
}
