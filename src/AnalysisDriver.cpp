#include "AnalysisDriver.h"

AnalysisDriver::AnalysisDriver(const tracer_state_t &tracer_state,
                               const std::string &output_dir,
                               const AnalysisSwitch analysis_switch)
    : analysis_switch_(analysis_switch),
      metadata_analysis_(MetadataAnalysis(tracer_state, output_dir)),
      object_count_size_analysis_(
          ObjectCountSizeAnalysis(tracer_state, output_dir)),
      function_analysis_(FunctionAnalysis(tracer_state, output_dir)),
      promise_type_analysis_(PromiseTypeAnalysis(tracer_state, output_dir)),
      promise_evaluation_distance_analysis_(
          PromiseEvaluationDistanceAnalysis(tracer_state, output_dir)),
      strictness_analysis_(StrictnessAnalysis(tracer_state, output_dir)),
      side_effect_analysis_(SideEffectAnalysis(tracer_state, output_dir)) {
    std::cout << "METADATA ANALYSIS          : " << analyze_metadata()
              << std::endl;
    std::cout << "OBJECT COUNT SIZE ANALYSIS : " << analyze_object_count_size()
              << std::endl;
    std::cout << "PROMISE ANALYSIS           : " << analyze_promises()
              << std::endl;
    std::cout << "FUNCTION ANALYSIS          : " << analyze_functions()
              << std::endl;
    std::cout << "STRICTNESS ANALYSIS        : " << analyze_strictness()
              << std::endl;
    std::cout << "SIDE EFFECT ANALYSIS       : " << analyze_side_effects()
              << std::endl;
}

void AnalysisDriver::begin(dyntrace_context_t *context) {}

void AnalysisDriver::promise_created(const prom_basic_info_t &prom_basic_info,
                                     const SEXP promise) {
    if (analyze_promises())
        promise_type_analysis_.promise_created(prom_basic_info, promise);
}

void AnalysisDriver::closure_entry(const closure_info_t &closure_info) {
    if (analyze_functions())
        function_analysis_.closure_entry(closure_info);
    if (analyze_promises())
        promise_type_analysis_.closure_entry(closure_info);
    if (analyze_strictness())
        strictness_analysis_.closure_entry(closure_info);
}

void AnalysisDriver::special_entry(const builtin_info_t &special_info) {
    if (analyze_functions())
        function_analysis_.special_entry(special_info);
}

void AnalysisDriver::builtin_entry(const builtin_info_t &builtin_info) {
    if (analyze_functions())
        function_analysis_.builtin_entry(builtin_info);
}

void AnalysisDriver::closure_exit(const closure_info_t &closure_info) {
    if (analyze_functions())
        function_analysis_.closure_exit(closure_info);
    if (analyze_strictness())
        strictness_analysis_.closure_exit(closure_info);
}

void AnalysisDriver::builtin_exit(const builtin_info_t &builtin_info) {
    if (analyze_functions())
        function_analysis_.builtin_exit(builtin_info);
}

void AnalysisDriver::special_exit(const builtin_info_t &special_info) {
    if (analyze_functions())
        function_analysis_.special_exit(special_info);
}

void AnalysisDriver::promise_force_entry(const prom_info_t &prom_info,
                                         const SEXP promise) {
    if (analyze_strictness())
        strictness_analysis_.promise_force_entry(prom_info, promise);
}

void AnalysisDriver::promise_force_exit(const prom_info_t &prom_info,
                                        const SEXP promise) {
    if (analyze_promises())
        promise_type_analysis_.promise_force_exit(prom_info, promise);
}

void AnalysisDriver::gc_promise_unmarked(const prom_id_t prom_id,
                                         const SEXP promise) {
    if (analyze_strictness())
        strictness_analysis_.gc_promise_unmarked(prom_id, promise);
    if (analyze_promises())
        promise_type_analysis_.gc_promise_unmarked(prom_id, promise);
}

void AnalysisDriver::promise_environment_lookup(const prom_info_t &info,
                                                const SEXP promise,
                                                int in_force) {
    if (analyze_strictness())
        strictness_analysis_.promise_environment_lookup(info, promise,
                                                        in_force);
}

void AnalysisDriver::promise_expression_lookup(const prom_info_t &info,
                                               const SEXP promise,
                                               int in_force) {
    if (analyze_strictness())
        strictness_analysis_.promise_expression_lookup(info, promise, in_force);
}

void AnalysisDriver::promise_value_lookup(const prom_info_t &info,
                                          const SEXP promise, int in_force) {
    if (analyze_strictness())
        strictness_analysis_.promise_value_lookup(info, promise, in_force);
}

void AnalysisDriver::promise_environment_set(const prom_info_t &info,
                                             const SEXP promise, int in_force) {
    if (analyze_strictness())
        strictness_analysis_.promise_environment_set(info, promise, in_force);
}

void AnalysisDriver::promise_expression_set(const prom_info_t &info,
                                            const SEXP promise, int in_force) {
    if (analyze_strictness())
        strictness_analysis_.promise_expression_set(info, promise, in_force);
}

void AnalysisDriver::promise_value_set(const prom_info_t &info,
                                       const SEXP promise, int in_force) {
    if (analyze_strictness())
        strictness_analysis_.promise_value_set(info, promise, in_force);
}

void AnalysisDriver::vector_alloc(const type_gc_info_t &type_gc_info) {
    if (analyze_object_count_size())
        object_count_size_analysis_.vector_alloc(type_gc_info);
}

void AnalysisDriver::environment_define_var(const SEXP symbol, const SEXP value,
                                            const SEXP rho) {
    if (analyze_side_effects())
        side_effect_analysis_.environment_define_var(symbol, value, rho);
}

void AnalysisDriver::environment_assign_var(const SEXP symbol, const SEXP value,
                                            const SEXP rho) {
    if (analyze_side_effects())
        side_effect_analysis_.environment_assign_var(symbol, value, rho);
}

void AnalysisDriver::environment_lookup_var(const SEXP symbol, const SEXP value,
                                            const SEXP rho) {
    if (analyze_side_effects())
        side_effect_analysis_.environment_lookup_var(symbol, value, rho);
}

void AnalysisDriver::environment_remove_var(const SEXP symbol, const SEXP rho) {
    if (analyze_side_effects())
        side_effect_analysis_.environment_remove_var(symbol, rho);
}

void AnalysisDriver::end(dyntrace_context_t *context) {
    if (analyze_metadata())
        metadata_analysis_.end(context);
    if (analyze_object_count_size())
        object_count_size_analysis_.end(context);
    if (analyze_functions())
        function_analysis_.end(context);
    if (analyze_promises())
        promise_type_analysis_.end(context);
    if (analyze_strictness())
        strictness_analysis_.end(context);
    if (analyze_side_effects())
        side_effect_analysis_.end(context);
}

inline bool AnalysisDriver::analyze_metadata() const {
    return analysis_switch_.metadata;
}

inline bool AnalysisDriver::analyze_object_count_size() const {
    return analysis_switch_.object_count_size;
}

inline bool AnalysisDriver::analyze_promises() const {
    return analysis_switch_.promise;
}

inline bool AnalysisDriver::analyze_functions() const {
    return analysis_switch_.function;
}

inline bool AnalysisDriver::analyze_strictness() const {
    return analysis_switch_.strictness;
}

inline bool AnalysisDriver::analyze_side_effects() const {
    return analysis_switch_.side_effect;
}
