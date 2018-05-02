#ifndef __STRICTNESS_ANALYSIS_H__
#define __STRICTNESS_ANALYSIS_H__

#include "ArgumentPromiseState.h"
#include "CallState.h"
#include "FunctionState.h"
#include "State.h"
#include <algorithm>
#include <tuple>
#include <unordered_map>
#include <vector>

class StrictnessAnalysis {
  public:
    StrictnessAnalysis(const tracer_state_t &tracer_state,
                       const std::string &output_dir);
    void closure_entry(const closure_info_t &closure_info);
    void closure_exit(const closure_info_t &closure_info);
    void promise_force_entry(const prom_info_t &prom_info, const SEXP promise);
    void promise_force_exit(const prom_info_t &prom_info, const SEXP promise);
    void gc_promise_unmarked(const prom_id_t &prom_id, const SEXP promise);
    void serialize();

  private:
    bool is_executing(call_id_t call_id);
    void update_argument_position(call_id_t call_id, fn_id_t fn_id,
                                  int position);
    void push_on_call_stack(CallState call_state);
    CallState pop_from_call_stack(call_id_t call_id);
    void serialize_function_formal_parameter_usage_count();
    void serialize_function_formal_parameter_usage_order();
    void serialize_function_call_count();

    std::unordered_map<fn_id_t, FunctionState> functions_;
    std::unordered_map<prom_id_t, ArgumentPromiseState> promises_;
    std::string output_dir_;
    std::vector<CallState> call_stack_;
    const tracer_state_t &tracer_state_;
};

#endif /* __STRICTNESS_ANALYSIS_H__ */
