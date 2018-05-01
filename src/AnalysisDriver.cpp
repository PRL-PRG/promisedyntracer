#include "AnalysisDriver.h"

AnalysisDriver::AnalysisDriver(const tracer_state_t &tracer_state,
                               const std::string &output_dir)
    : strictness_analysis_(StrictnessAnalysis(tracer_state, output_dir)),
      // object_count_size_analysis_(
      //          ObjectCountSizeAnalysis(tracer_state, output_dir)),
      promise_type_analysis_(PromiseTypeAnalysis(tracer_state, output_dir)),
      promise_evaluation_distance_analysis_(
          PromiseEvaluationDistanceAnalysis(tracer_state, output_dir)),
      output_dir_(output_dir) {}

void AnalysisDriver::promise_created(const prom_basic_info_t &prom_basic_info,
                                     const SEXP promise) {
    promise_type_analysis_.promise_created(prom_basic_info, promise);
    promise_evaluation_distance_analysis_.promise_created(prom_basic_info,
                                                          promise);
}

void AnalysisDriver::closure_entry(const closure_info_t &closure_info) {
    strictness_analysis_.closure_entry(closure_info);
    promise_type_analysis_.closure_entry(closure_info);
    promise_evaluation_distance_analysis_.closure_entry(closure_info);
}

void AnalysisDriver::special_entry(const builtin_info_t &builtin_info) {
    promise_evaluation_distance_analysis_.special_entry(builtin_info);
}

void AnalysisDriver::builtin_entry(const builtin_info_t &builtin_info) {
    promise_evaluation_distance_analysis_.builtin_entry(builtin_info);
}

void AnalysisDriver::closure_exit(const closure_info_t &closure_info) {
    strictness_analysis_.closure_exit(closure_info);
}

void AnalysisDriver::promise_force_entry(const prom_info_t &prom_info) {
    strictness_analysis_.promise_force_entry(prom_info);
    promise_evaluation_distance_analysis_.promise_force_entry(prom_info);
}

void AnalysisDriver::promise_force_exit(const prom_info_t &prom_info,
                                        const SEXP promise) {
    strictness_analysis_.promise_force_exit(prom_info, promise);
    promise_type_analysis_.promise_force_exit(prom_info, promise);
}

void AnalysisDriver::gc_promise_unmarked(const prom_id_t &prom_id,
                                         const SEXP promise) {
    strictness_analysis_.gc_promise_unmarked(prom_id, promise);
    promise_type_analysis_.gc_promise_unmarked(prom_id, promise);
    promise_evaluation_distance_analysis_.gc_promise_unmarked(prom_id, promise);
}

void AnalysisDriver::vector_alloc(const type_gc_info_t &type_gc_info) {
    // object_count_size_analysis_.vector_alloc(type_gc_info);
}

void AnalysisDriver::serialize() {
    strictness_analysis_.serialize();
    // object_count_size_analysis_.serialize();
    promise_type_analysis_.serialize();
    promise_evaluation_distance_analysis_.serialize();
}
