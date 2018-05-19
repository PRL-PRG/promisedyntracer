#ifndef __PROMISE_EVALUATION_ANALYSIS_H__
#define __PROMISE_EVALUATION_ANALYSIS_H__

#include "PromiseMapper.h"
#include "PromiseState.h"
#include "State.h"
#include <algorithm>
#include <tuple>
#include <unordered_map>
#include <vector>

class PromiseEvaluationAnalysis {
  public:
    enum class EvaluationContext {
        PROMISE = 0,
        CLOSURE,
        SPECIAL,
        BUILTIN,
        GLOBAL,
        COUNT
    };

    PromiseEvaluationAnalysis(tracer_state_t &tracer_state,
                              const std::string &output_dir,
                              PromiseMapper *promise_mapper);
    void promise_force_entry(const prom_info_t &prom_info, const SEXP promise);
    void end(dyntrace_context_t *context);

  private:
    void serialize();
    void serialize_promise_evaluation_distance();
    void serialize_evaluation_context_count();
    void compute_evaluation_distance(const PromiseState &promise_state);
    void update_evaluation_distance(std::string key);
    EvaluationContext get_current_evaluation_context();
    void update_evaluation_context_count(EvaluationContext evalution_context);
    std::unordered_map<std::string, int> evaluation_distances_;
    std::vector<int> evaluation_context_counts_;
    std::string output_dir_;
    tracer_state_t &tracer_state_;
    PromiseMapper *promise_mapper_;
};

std::string
to_string(PromiseEvaluationAnalysis::EvaluationContext evaluation_context);

#endif /* __PROMISE_EVALUATION_ANALYSIS_H__ */
