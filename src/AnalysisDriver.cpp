#include "AnalysisDriver.h"

AnalysisDriver::AnalysisDriver(tracer_state_t &tracer_state,
                               const std::string &output_dir,
                               const AnalysisSwitch analysis_switch)
    : analysis_switch_(analysis_switch),
      promise_mapper_(PromiseMapper(tracer_state, output_dir)),
      metadata_analysis_(MetadataAnalysis(tracer_state, output_dir)),
      object_count_size_analysis_(
          ObjectCountSizeAnalysis(tracer_state, output_dir)),
      function_analysis_(FunctionAnalysis(tracer_state, output_dir)),
      promise_evaluation_analysis_(PromiseEvaluationAnalysis(
          tracer_state, output_dir, &promise_mapper_)),
      promise_type_analysis_(PromiseTypeAnalysis(tracer_state, output_dir)),
      strictness_analysis_(
          StrictnessAnalysis(tracer_state, output_dir, &promise_mapper_)),
      side_effect_analysis_(SideEffectAnalysis(tracer_state, output_dir)) {

    std::cout << analysis_switch;
}

void AnalysisDriver::begin(dyntracer_t *dyntracer) {}

void AnalysisDriver::promise_created(const prom_basic_info_t &prom_basic_info,
                                     const SEXP promise) {
    ANALYSIS_TIMER_RESET();

    // WARNING: This has to be at the beginning. This adds promises to the
    // mapping. These promises are used by the analysis below.
    if (map_promises())
        promise_mapper_.promise_created(prom_basic_info, promise);

    ANALYSIS_TIMER_END_SEGMENT(CREATE_PROMISE_ANALYSIS_PROMISE_MAPPER);

    if (analyze_promise_types())
        promise_type_analysis_.promise_created(prom_basic_info, promise);

    ANALYSIS_TIMER_END_SEGMENT(CREATE_PROMISE_ANALYSIS_PROMISE_TYPE);
}

void AnalysisDriver::closure_entry(const closure_info_t &closure_info) {
    ANALYSIS_TIMER_RESET();

    // WARNING: This has to be at the beginning. This updates promises
    // in the mapping. These promises are used by the analysis below.
    if (map_promises())
        promise_mapper_.closure_entry(closure_info);

    ANALYSIS_TIMER_END_SEGMENT(FUNCTION_ENTRY_ANALYSIS_PROMISE_MAPPER);

    if (analyze_functions())
        function_analysis_.closure_entry(closure_info);

    ANALYSIS_TIMER_END_SEGMENT(FUNCTION_ENTRY_ANALYSIS_FUNCTION);

    if (analyze_promise_types())
        promise_type_analysis_.closure_entry(closure_info);

    ANALYSIS_TIMER_END_SEGMENT(FUNCTION_ENTRY_ANALYSIS_PROMISE_TYPE);

    if (analyze_strictness())
        strictness_analysis_.closure_entry(closure_info);

    ANALYSIS_TIMER_END_SEGMENT(FUNCTION_ENTRY_ANALYSIS_STRICTNESS);
}

void AnalysisDriver::special_entry(const builtin_info_t &special_info) {
    ANALYSIS_TIMER_RESET();

    if (analyze_functions())
        function_analysis_.special_entry(special_info);

    ANALYSIS_TIMER_END_SEGMENT(BUILTIN_ENTRY_ANALYSIS_FUNCTION);
}

void AnalysisDriver::builtin_entry(const builtin_info_t &builtin_info) {
    ANALYSIS_TIMER_RESET();

    if (analyze_functions())
        function_analysis_.builtin_entry(builtin_info);

    ANALYSIS_TIMER_END_SEGMENT(BUILTIN_ENTRY_ANALYSIS_FUNCTION);
}

void AnalysisDriver::closure_exit(const closure_info_t &closure_info) {
    ANALYSIS_TIMER_RESET();

    if (analyze_functions())
        function_analysis_.closure_exit(closure_info);

    ANALYSIS_TIMER_END_SEGMENT(FUNCTION_EXIT_ANALYSIS_FUNCTION);

    if (analyze_strictness())
        strictness_analysis_.closure_exit(closure_info);

    ANALYSIS_TIMER_END_SEGMENT(FUNCTION_EXIT_ANALYSIS_STRICTNESS);
}

void AnalysisDriver::builtin_exit(const builtin_info_t &builtin_info) {
    ANALYSIS_TIMER_RESET();

    if (analyze_functions())
        function_analysis_.builtin_exit(builtin_info);

    ANALYSIS_TIMER_END_SEGMENT(BUILTIN_EXIT_ANALYSIS_FUNCTION);
}

void AnalysisDriver::special_exit(const builtin_info_t &special_info) {
    ANALYSIS_TIMER_RESET();

    if (analyze_functions())
        function_analysis_.special_exit(special_info);

    ANALYSIS_TIMER_END_SEGMENT(BUILTIN_EXIT_ANALYSIS_FUNCTION);
}

void AnalysisDriver::promise_force_entry(const prom_info_t &prom_info,
                                         const SEXP promise) {
    ANALYSIS_TIMER_RESET();

    if (map_promises())
        promise_mapper_.promise_force_entry(prom_info, promise);

    ANALYSIS_TIMER_END_SEGMENT(FORCE_PROMISE_ENTRY_ANALYSIS_PROMISE_MAPPER);

    if (analyze_promise_evaluations())
        promise_evaluation_analysis_.promise_force_entry(prom_info, promise);

    ANALYSIS_TIMER_END_SEGMENT(
        FORCE_PROMISE_ENTRY_ANALYSIS_PROMISE_EVALUATION_DISTANCE);

    if (analyze_strictness())
        strictness_analysis_.promise_force_entry(prom_info, promise);

    ANALYSIS_TIMER_END_SEGMENT(FORCE_PROMISE_ENTRY_ANALYSIS_STRICTNESS);
}

void AnalysisDriver::promise_force_exit(const prom_info_t &prom_info,
                                        const SEXP promise) {
    ANALYSIS_TIMER_RESET();

    if (analyze_promise_types())
        promise_type_analysis_.promise_force_exit(prom_info, promise);

    ANALYSIS_TIMER_END_SEGMENT(FORCE_PROMISE_EXIT_ANALYSIS_PROMISE_TYPE);
}

void AnalysisDriver::gc_promise_unmarked(const prom_id_t prom_id,
                                         const SEXP promise) {
    ANALYSIS_TIMER_RESET();

    if (analyze_strictness())
        strictness_analysis_.gc_promise_unmarked(prom_id, promise);

    ANALYSIS_TIMER_END_SEGMENT(GC_PROMISE_UNMARKED_ANALYSIS_STRICTNESS);

    if (analyze_promise_types())
        promise_type_analysis_.gc_promise_unmarked(prom_id, promise);

    ANALYSIS_TIMER_END_SEGMENT(GC_PROMISE_UNMARKED_ANALYSIS_PROMISE_TYPE);

    // WARNING: This has to be at the end. This removes promises from the
    // mapping. These promises are used by the analysis above.
    if (map_promises())
        promise_mapper_.gc_promise_unmarked(prom_id, promise);

    ANALYSIS_TIMER_END_SEGMENT(GC_PROMISE_UNMARKED_ANALYSIS_PROMISE_MAPPER);
}

void AnalysisDriver::promise_environment_lookup(const prom_info_t &info,
                                                const SEXP promise,
                                                int in_force) {
    ANALYSIS_TIMER_RESET();

    if (map_promises())
        promise_mapper_.promise_environment_lookup(info, promise, in_force);

    ANALYSIS_TIMER_END_SEGMENT(
        LOOKUP_PROMISE_ENVIRONMENT_ANALYSIS_PROMISE_MAPPER);

    // if (analyze_strictness())
    //     strictness_analysis_.promise_environment_lookup(info, promise,
    //                                                     in_force);

    ANALYSIS_TIMER_END_SEGMENT(LOOKUP_PROMISE_ENVIRONMENT_ANALYSIS_STRICTNESS);
}

void AnalysisDriver::promise_expression_lookup(const prom_info_t &info,
                                               const SEXP promise,
                                               int in_force) {
    ANALYSIS_TIMER_RESET();

    if (map_promises())
        promise_mapper_.promise_expression_lookup(info, promise, in_force);

    ANALYSIS_TIMER_END_SEGMENT(
        LOOKUP_PROMISE_EXPRESSION_ANALYSIS_PROMISE_MAPPER);

    // if (analyze_strictness())
    //     strictness_analysis_.promise_expression_lookup(info, promise,
    //     in_force);

    ANALYSIS_TIMER_END_SEGMENT(LOOKUP_PROMISE_EXPRESSION_ANALYSIS_STRICTNESS);
}

void AnalysisDriver::promise_value_lookup(const prom_info_t &info,
                                          const SEXP promise, int in_force) {
    ANALYSIS_TIMER_RESET();

    if (map_promises())
        promise_mapper_.promise_value_lookup(info, promise, in_force);

    ANALYSIS_TIMER_END_SEGMENT(LOOKUP_PROMISE_VALUE_ANALYSIS_PROMISE_MAPPER);

    // if (analyze_strictness())
    //     strictness_analysis_.promise_value_lookup(info, promise, in_force);

    ANALYSIS_TIMER_END_SEGMENT(LOOKUP_PROMISE_VALUE_ANALYSIS_STRICTNESS);
}

void AnalysisDriver::promise_environment_set(const prom_info_t &info,
                                             const SEXP promise, int in_force) {
    ANALYSIS_TIMER_RESET();

    if (map_promises())
        promise_mapper_.promise_environment_set(info, promise, in_force);

    ANALYSIS_TIMER_END_SEGMENT(SET_PROMISE_ENVIRONMENT_ANALYSIS_PROMISE_MAPPER);

    // if (analyze_strictness())
    //     strictness_analysis_.promise_environment_set(info, promise,
    //     in_force);

    ANALYSIS_TIMER_END_SEGMENT(SET_PROMISE_ENVIRONMENT_ANALYSIS_STRICTNESS);
}

void AnalysisDriver::promise_expression_set(const prom_info_t &info,
                                            const SEXP promise, int in_force) {
    ANALYSIS_TIMER_RESET();

    if (map_promises())
        promise_mapper_.promise_expression_set(info, promise, in_force);

    ANALYSIS_TIMER_END_SEGMENT(SET_PROMISE_EXPRESSION_ANALYSIS_PROMISE_MAPPER);

    // if (analyze_strictness())
    //     strictness_analysis_.promise_expression_set(info, promise, in_force);

    ANALYSIS_TIMER_END_SEGMENT(SET_PROMISE_EXPRESSION_ANALYSIS_STRICTNESS);
}

void AnalysisDriver::promise_value_set(const prom_info_t &info,
                                       const SEXP promise, int in_force) {
    ANALYSIS_TIMER_RESET();

    if (map_promises())
        promise_mapper_.promise_value_set(info, promise, in_force);

    ANALYSIS_TIMER_END_SEGMENT(SET_PROMISE_VALUE_ANALYSIS_PROMISE_MAPPER);

    // if (analyze_strictness())
    //     strictness_analysis_.promise_value_set(info, promise, in_force);

    ANALYSIS_TIMER_END_SEGMENT(SET_PROMISE_VALUE_ANALYSIS_STRICTNESS);
}

void AnalysisDriver::vector_alloc(const type_gc_info_t &type_gc_info) {
    ANALYSIS_TIMER_RESET();

    if (analyze_object_count_size())
        object_count_size_analysis_.vector_alloc(type_gc_info);

    ANALYSIS_TIMER_END_SEGMENT(VECTOR_ALLOC_ANALYSIS_OBJECT_COUNT_SIZE);
}

void AnalysisDriver::environment_define_var(const SEXP symbol, const SEXP value,
                                            const SEXP rho) {
    ANALYSIS_TIMER_RESET();

    if (analyze_side_effects())
        side_effect_analysis_.environment_define_var(symbol, value, rho);

    ANALYSIS_TIMER_END_SEGMENT(ENVIRONMENT_ACTION_ANALYSIS_SIDE_EFFECT);
}

void AnalysisDriver::environment_assign_var(const SEXP symbol, const SEXP value,
                                            const SEXP rho) {
    ANALYSIS_TIMER_RESET();

    if (analyze_side_effects())
        side_effect_analysis_.environment_assign_var(symbol, value, rho);

    ANALYSIS_TIMER_END_SEGMENT(ENVIRONMENT_ACTION_ANALYSIS_SIDE_EFFECT);
}

void AnalysisDriver::environment_lookup_var(const SEXP symbol, const SEXP value,
                                            const SEXP rho) {
    ANALYSIS_TIMER_RESET();

    if (analyze_side_effects())
        side_effect_analysis_.environment_lookup_var(symbol, value, rho);

    ANALYSIS_TIMER_END_SEGMENT(ENVIRONMENT_ACTION_ANALYSIS_SIDE_EFFECT);
}

void AnalysisDriver::environment_remove_var(const SEXP symbol, const SEXP rho) {
    ANALYSIS_TIMER_RESET();

    if (analyze_side_effects())
        side_effect_analysis_.environment_remove_var(symbol, rho);

    ANALYSIS_TIMER_END_SEGMENT(ENVIRONMENT_ACTION_ANALYSIS_SIDE_EFFECT);
}

void AnalysisDriver::context_jump(const unwind_info_t &info) {
    ANALYSIS_TIMER_RESET();

    if (analyze_strictness())
        strictness_analysis_.context_jump(info);

    ANALYSIS_TIMER_END_SEGMENT(CONTEXT_JUMP_ANALYSIS_STRICTNESS);
}

void AnalysisDriver::end(dyntracer_t *dyntracer) {
    ANALYSIS_TIMER_RESET();

    if (analyze_metadata())
        metadata_analysis_.end(dyntracer);

    ANALYSIS_TIMER_END_SEGMENT(END_ANALYSIS_METADATA);

    if (analyze_object_count_size())
        object_count_size_analysis_.end(dyntracer);

    ANALYSIS_TIMER_END_SEGMENT(END_ANALYSIS_OBJECT_COUNT_SIZE);

    if (analyze_functions())
        function_analysis_.end(dyntracer);

    ANALYSIS_TIMER_END_SEGMENT(END_ANALYSIS_FUNCTION);

    if (analyze_promise_types())
        promise_type_analysis_.end(dyntracer);

    ANALYSIS_TIMER_END_SEGMENT(END_ANALYSIS_PROMISE_TYPE);

    if (analyze_promise_evaluations())
        promise_evaluation_analysis_.end(dyntracer);

    ANALYSIS_TIMER_END_SEGMENT(END_ANALYSIS_PROMISE_EVALUATION_DISTANCE);

    if (analyze_strictness())
        strictness_analysis_.end(dyntracer);

    ANALYSIS_TIMER_END_SEGMENT(END_ANALYSIS_STRICTNESS);

    if (analyze_side_effects())
        side_effect_analysis_.end(dyntracer);

    ANALYSIS_TIMER_END_SEGMENT(END_ANALYSIS_SIDE_EFFECT);

    // WARNING: This has to be at the end. This removes promises from
    // the mapping. These promises are used by the analysis above.
    if (map_promises())
        promise_mapper_.end(dyntracer);

    ANALYSIS_TIMER_END_SEGMENT(END_ANALYSIS_PROMISE_MAPPER);
}

inline bool AnalysisDriver::analyze_metadata() const {
    return analysis_switch_.metadata;
}

inline bool AnalysisDriver::analyze_object_count_size() const {
    return analysis_switch_.object_count_size;
}

inline bool AnalysisDriver::analyze_promise_types() const {
    return analysis_switch_.promise_type;
}

inline bool AnalysisDriver::analyze_promise_slot_mutations() const {
    return analysis_switch_.promise_slot_mutation;
}

inline bool AnalysisDriver::analyze_promise_evaluations() const {
    return analysis_switch_.promise_evaluation;
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

inline bool AnalysisDriver::map_promises() const {
    return analysis_switch_.strictness || analysis_switch_.promise_evaluation ||
           analysis_switch_.promise_slot_mutation;
}
