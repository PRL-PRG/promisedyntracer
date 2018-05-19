#include "PromiseEvaluationAnalysis.h"

PromiseEvaluationAnalysis::PromiseEvaluationAnalysis(
    tracer_state_t &tracer_state, const std::string &output_dir,
    PromiseMapper *promise_mapper)
    : tracer_state_(tracer_state), output_dir_(output_dir),
      promise_mapper_(promise_mapper),
      evaluation_context_counts_(std::vector<int>(to_underlying_type(
          PromiseEvaluationAnalysis::EvaluationContext::COUNT))) {}

void PromiseEvaluationAnalysis::promise_force_entry(
    const prom_info_t &prom_info, const SEXP promise) {

    PromiseState &promise_state = promise_mapper_->find(prom_info.prom_id);

    promise_state.evaluated = true;

    update_evaluation_context_count(get_current_evaluation_context());

    if (promise_state.local && promise_state.argument)
        compute_evaluation_distance(promise_state);
}

void PromiseEvaluationAnalysis::compute_evaluation_distance(
    const PromiseState &promise_state) {

    std::string key;
    std::string argument_type = promise_state.default_argument ? "da" : "ca";

    int closure_count = 0;
    int builtin_count = 0;
    int special_count = 0;
    int promise_count = 0;

    size_t stack_size = tracer_state_.full_stack.size();
    for (int i = stack_size - 1; i >= 0; --i) {
        stack_event_t exec_context = tracer_state_.full_stack[i];
        if (exec_context.type == stack_type::CALL) {
            SEXP enclosing_address =
                reinterpret_cast<SEXP>(exec_context.enclosing_environment);
            if (promise_state.env_id ==
                tracer_state_.to_environment_id(enclosing_address)) {
                std::string key = argument_type + " , " +
                                  std::to_string(closure_count) + " , " +
                                  std::to_string(special_count) + " , " +
                                  std::to_string(builtin_count) + " , " +
                                  std::to_string(promise_count) + " , ";
                update_evaluation_distance(key);
                return;
            } else if (exec_context.function_info.type ==
                       function_type::CLOSURE)
                closure_count++;
            else if (exec_context.function_info.type == function_type::SPECIAL)
                special_count++;
            else
                builtin_count++;
        } else if (exec_context.type == stack_type::PROMISE) {
            promise_count++;
        }
    }
}

void PromiseEvaluationAnalysis::update_evaluation_distance(std::string key) {
    auto result = evaluation_distances_.emplace(make_pair(key, 1));
    if (!result.second)
        ++result.first->second;
}

void PromiseEvaluationAnalysis::end(dyntrace_context_t *context) {
    serialize();
}

PromiseEvaluationAnalysis::EvaluationContext
PromiseEvaluationAnalysis::get_current_evaluation_context() {

    size_t stack_size = tracer_state_.full_stack.size();
    for (int i = stack_size - 1; i >= 0; --i) {
        stack_event_t exec_context = tracer_state_.full_stack[i];
        if (exec_context.type == stack_type::CALL) {
            if (exec_context.function_info.type == function_type::CLOSURE)
                return EvaluationContext::CLOSURE;
            else if (exec_context.function_info.type == function_type::SPECIAL)
                return EvaluationContext::SPECIAL;
            else
                return EvaluationContext::BUILTIN;

        } else if (exec_context.type == stack_type::PROMISE) {
            return EvaluationContext::CLOSURE;
        }
    }
    return EvaluationContext::GLOBAL;
}

void PromiseEvaluationAnalysis::update_evaluation_context_count(
    EvaluationContext evaluation_context) {
    ++evaluation_context_counts_[to_underlying_type(evaluation_context)];
}

void PromiseEvaluationAnalysis::serialize() {
    serialize_promise_evaluation_distance();
    serialize_evaluation_context_count();
}

void PromiseEvaluationAnalysis::serialize_promise_evaluation_distance() {
    std::ofstream fout(output_dir_ + "/promise-evaluation-distance.csv",
                       std::ios::trunc);

    fout << "promise_type"
         << " , "
         << "closure_count"
         << " , "
         << "special_count"
         << " , "
         << "builtin_count"
         << " , "
         << "promise_count"
         << " , "
         << "promise_count" << std::endl;

    for (const auto &key_value : evaluation_distances_) {
        fout << key_value.first << " , " << key_value.second << std::endl;
    }

    fout.close();
}

void PromiseEvaluationAnalysis::serialize_evaluation_context_count() {

    std::ofstream fout(output_dir_ + "/promise-evaluation-context.csv",
                       std::ios::trunc);
    fout << "context"
         << " , "
         << "promise_count" << std::endl;

    for (int i = 0; i < evaluation_context_counts_.size(); ++i) {
        fout << to_string(static_cast<EvaluationContext>(i)) << " , "
             << evaluation_context_counts_[i] << std::endl;
    }

    fout.close();
}

std::string
to_string(PromiseEvaluationAnalysis::EvaluationContext evaluation_context) {
    switch (evaluation_context) {
        case PromiseEvaluationAnalysis::EvaluationContext::PROMISE:
            return "PROMISE";
        case PromiseEvaluationAnalysis::EvaluationContext::CLOSURE:
            return "CLOSURE";
        case PromiseEvaluationAnalysis::EvaluationContext::SPECIAL:
            return "SPECIAL";
        case PromiseEvaluationAnalysis::EvaluationContext::BUILTIN:
            return "BUILTIN";
        case PromiseEvaluationAnalysis::EvaluationContext::GLOBAL:
            return "GLOBAL";
        case PromiseEvaluationAnalysis::EvaluationContext::COUNT:
            return "COUNT";
    }
}
