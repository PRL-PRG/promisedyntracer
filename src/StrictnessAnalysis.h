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
    const static int UNINITIALIZED;
    const static int PROMISE;
    const static int CLOSURE;
    const static int SPECIAL;
    const static int BUILTIN;
    const static int GLOBAL;
    const static std::vector<std::string> evaluation_contexts;

    StrictnessAnalysis(const tracer_state_t &tracer_state,
                       const std::string &output_dir);
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
    void serialize();

  private:
    bool is_executing(call_id_t call_id);
    void update_argument_position(call_id_t call_id, fn_id_t fn_id,
                                  int position);
    void push_on_call_stack(CallState call_state);
    CallState pop_from_call_stack(call_id_t call_id);
    void compute_evaluation_distance(const SEXP promise,
                                     const ArgumentPromiseState &promise_state);
    void update_evaluation_distance(std::string key);
    int compute_immediate_parent();
    void update_evaluation_context_count(int evaluation_context);

    void update_promise_argument_slot(const prom_id_t prom_id, const int slot,
                                      int in_force);
    void
    update_promise_slot_access_count(const ArgumentPromiseState &promise_state);

    void serialize_function_formal_parameter_usage_count();
    void serialize_function_formal_parameter_usage_order();
    void serialize_function_call_count();
    void serialize_evaluation_distance();
    void serialize_evaluation_context_counts();
    void serialize_promise_slot_accesses();

    std::unordered_map<fn_id_t, FunctionState> functions_;
    std::unordered_map<prom_id_t, ArgumentPromiseState> promises_;
    std::unordered_map<std::string, int> evaluation_distances_;
    std::unordered_map<std::string, int> promise_slot_accesses_;
    std::string output_dir_;
    std::vector<CallState> call_stack_;
    std::vector<int> evaluation_context_counts_;
    const tracer_state_t &tracer_state_;
};

#endif /* __STRICTNESS_ANALYSIS_H__ */
