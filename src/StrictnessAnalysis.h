#ifndef __STRICTNESS_ANALYSIS_H__
#define __STRICTNESS_ANALYSIS_H__

#include "CallState.h"
#include "FunctionState.h"
#include "PromiseMapper.h"
#include "State.h"
#include <algorithm>
#include <tuple>
#include <unordered_map>
#include <vector>

class StrictnessAnalysis {
  public:
    StrictnessAnalysis(const tracer_state_t &tracer_state,
                       const std::string &output_dir,
                       PromiseMapper *const promise_mapper);
    void closure_entry(const closure_info_t &closure_info);
    void closure_exit(const closure_info_t &closure_info);
    void promise_force_entry(const prom_info_t &prom_info, const SEXP promise);
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
    void context_jump(const unwind_info_t &info);
    void end(dyntracer_t *dyntracer);

  private:
    bool is_executing(call_id_t call_id);
    void update_argument_position(call_id_t call_id, fn_id_t fn_id,
                                  int position);
    void push_on_call_stack(CallState call_state);
    CallState pop_from_call_stack(call_id_t call_id);

    void update_promise_argument_slot(const prom_id_t prom_id,
                                      PromiseState::SlotMutation slot_mutation,
                                      int in_force);
    void update_promise_slot_access_count(const PromiseState &promise_state);
    void remove_stack_frame(call_id_t call_id, fn_id_t fn_id);
    void serialize();
    void serialize_function_formal_parameter_usage_count();
    void serialize_function_formal_parameter_usage_order();
    void serialize_function_call_count();
    void serialize_evaluation_context_counts();
    void serialize_promise_slot_accesses();

    std::unordered_map<fn_id_t, FunctionState> functions_;
    std::string output_dir_;
    const tracer_state_t &tracer_state_;

    std::vector<CallState> call_stack_;
    PromiseMapper *const promise_mapper_;
};

#endif /* __STRICTNESS_ANALYSIS_H__ */
