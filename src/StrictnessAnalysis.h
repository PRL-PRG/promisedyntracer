#ifndef __STRICTNESS_ANALYSIS_H__
#define __STRICTNESS_ANALYSIS_H__

#include "BinaryTableWriter.h"
#include "BinaryTableWriter.h"
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
    void promise_value_lookup(const prom_info_t &prom_info, const SEXP promise);
    void promise_value_assign(const prom_info_t &info, const SEXP promise);
    void promise_environment_lookup(const prom_info_t &prom_info,
                                    const SEXP promise);
    void promise_environment_assign(const prom_info_t &prom_info,
                                    const SEXP promise);
    void promise_expression_lookup(const prom_info_t &prom_info,
                                   const SEXP promise);
    void promise_expression_assign(const prom_info_t &prom_info,
                                   const SEXP promise);
    void context_jump(const unwind_info_t &info);
    void end(dyntracer_t *dyntracer);

  private:
    void push_on_call_stack(CallState call_state);
    CallState pop_from_call_stack(call_id_t call_id);

    void update_promise_argument_slot(const prom_id_t prom_id,
                                      PromiseState::SlotMutation slot_mutation);
    void update_promise_slot_access_count(const PromiseState &promise_state);
    void remove_stack_frame(call_id_t call_id, fn_id_t fn_id);
    void serialize();
    void serialize_parameter_usage_order();
    void serialize_parameter_usage_count(const CallState &call_state);
    void serialize_function_call_count();
    void serialize_evaluation_context_counts();
    void serialize_promise_slot_accesses();

    void metaprogram_(const prom_info_t &prom_info, const SEXP promise);
    CallState *get_call_state(const call_id_t call_id);

    std::unordered_map<fn_id_t, FunctionState> functions_;
    std::string output_dir_;
    const tracer_state_t &tracer_state_;

    std::vector<CallState> call_stack_;
    PromiseMapper *const promise_mapper_;
    BinaryTableWriter usage_table_writer_;
    BinaryTableWriter order_table_writer_;
};

#endif /* __STRICTNESS_ANALYSIS_H__ */
