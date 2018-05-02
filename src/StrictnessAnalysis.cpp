#include "StrictnessAnalysis.h"

const size_t FUNCTION_MAPPING_BUCKET_SIZE = 20000;
const size_t PROMISE_MAPPING_BUCKET_SIZE = 20000000;

const int StrictnessAnalysis::UNINITIALIZED = 0;
const int StrictnessAnalysis::PROMISE = 1;
const int StrictnessAnalysis::CLOSURE = 2;
const int StrictnessAnalysis::SPECIAL = 3;
const int StrictnessAnalysis::BUILTIN = 4;
const int StrictnessAnalysis::GLOBAL = 5;

const std::vector<std::string> StrictnessAnalysis::evaluation_contexts{
    "U", "P", "C", "S", "B", "G"};

StrictnessAnalysis::StrictnessAnalysis(const tracer_state_t &tracer_state,
                                       const std::string &output_dir)
    : tracer_state_(tracer_state), output_dir_(output_dir),
      functions_(std::unordered_map<fn_id_t, FunctionState>(
          FUNCTION_MAPPING_BUCKET_SIZE)),
      promises_(std::unordered_map<prom_id_t, ArgumentPromiseState>(
          PROMISE_MAPPING_BUCKET_SIZE)),
      evaluation_context_counts_(
          std::vector<int>(StrictnessAnalysis::evaluation_contexts.size())) {}

void StrictnessAnalysis::closure_entry(const closure_info_t &closure_info) {

    // push call_id to call_stack
    call_id_t call_id = closure_info.call_id;
    fn_id_t fn_id = closure_info.fn_id;

    push_on_call_stack(CallState(call_id, fn_id));

    int max_position = 0;
    int actual_argument_position = 0;
    for (const auto& argument : closure_info.arguments) {
        prom_id_t promise_id = argument.promise_id;
        int formal_parameter_position = argument.formal_parameter_position;
        bool is_default_argument = argument.default_argument;
        auto result = promises_.insert(
            {promise_id,
             ArgumentPromiseState(fn_id, call_id, formal_parameter_position,
                                  actual_argument_position, is_default_argument,
                                  StrictnessAnalysis::UNINITIALIZED)});
        assert(result.second == true);
        if (formal_parameter_position > max_position)
            max_position = formal_parameter_position;
        actual_argument_position++;
    }

    auto fn_iter = functions_.insert(
        std::make_pair(fn_id, FunctionState(max_position + 1)));
    fn_iter.first->second.increment_call();
}

void StrictnessAnalysis::closure_exit(const closure_info_t &closure_info) {
    // pop call_id from call_stack
    CallState call_state = pop_from_call_stack(closure_info.call_id);
    auto it = functions_.find(closure_info.fn_id);
    assert(it != functions_.end());
    it->second.add_formal_parameter_usage_order(
        call_state.formal_parameter_usage_order);
}

void StrictnessAnalysis::promise_force_entry(const prom_info_t &prom_info,
                                             const SEXP promise) {
    prom_id_t prom_id = prom_info.prom_id;
    auto iter = promises_.find(prom_id);
    if (iter == promises_.end())
        return;
    iter->second.evaluated = true;
    compute_evaluation_distance(promise, iter->second);

    iter->second.evaluation_context = compute_immediate_parent();
    update_evaluation_context_count(iter->second.evaluation_context);
    call_id_t call_id = iter->second.call_id;
    if (!is_executing(call_id))
        return;
    fn_id_t fn_id = iter->second.fn_id;
    int formal_parameter_position = iter->second.formal_parameter_position;

    update_argument_position(call_id, fn_id, formal_parameter_position);
}

int StrictnessAnalysis::compute_immediate_parent() {

    size_t stack_size = tracer_state_.full_stack.size();
    for (int i = stack_size - 1; i >= 0; --i) {
        stack_event_t exec_context = tracer_state_.full_stack[i];
        if (exec_context.type == stack_type::CALL) {
            if (exec_context.function_info.type == function_type::CLOSURE)
                return StrictnessAnalysis::CLOSURE;
            else if (exec_context.function_info.type == function_type::SPECIAL)
                return StrictnessAnalysis::SPECIAL;
            else
                return StrictnessAnalysis::BUILTIN;

        } else if (exec_context.type == stack_type::PROMISE) {
            return StrictnessAnalysis::PROMISE;
        }
    }
    return StrictnessAnalysis::GLOBAL;
}

void StrictnessAnalysis::update_evaluation_context_count(
    int evaluation_context) {
    ++evaluation_context_counts_[evaluation_context];
}

void StrictnessAnalysis::compute_evaluation_distance(
    const SEXP promise, const ArgumentPromiseState &promise_state) {

    std::string key;
    std::string argument_type = promise_state.default_argument ? "da" : "ca";

    if (promise_state.evaluated == false) {
        update_evaluation_distance("Inf , Inf , Inf , Inf , " + argument_type +
                                   " , N");
        return;
    }

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
            // function side effects matter iff they are done in external
            // environments.
            if (PRENV(promise) == enclosing_address) {
                std::string key = std::to_string(closure_count) + " , " +
                                  std::to_string(special_count) + " , " +
                                  std::to_string(builtin_count) + " , " +
                                  std::to_string(promise_count) + " , " +
                                  argument_type + " , " + "Y";

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

    update_evaluation_distance("Inf , Inf , Inf , Inf , " + argument_type +
                               " , Y");
}

void StrictnessAnalysis::update_evaluation_distance(std::string key) {
    auto result = evaluation_distances_.emplace(make_pair(key, 1));
    if (!result.second)
        ++result.first->second;
}

void StrictnessAnalysis::gc_promise_unmarked(const prom_id_t prom_id,
                                             const SEXP promise) {
    auto iter = promises_.find(prom_id);
    if (iter == promises_.end())
        return;
    compute_evaluation_distance(promise, iter->second);

    promises_.erase(prom_id);
}

bool StrictnessAnalysis::is_executing(call_id_t call_id) {
    for (auto it = call_stack_.crbegin(); it != call_stack_.crend(); ++it) {
        if (it->call_id == call_id) {
            return true;
        }
    }
    return false;
}

void StrictnessAnalysis::update_argument_position(
    call_id_t call_id, fn_id_t fn_id, int formal_parameter_position) {
    auto it = functions_.find(fn_id);
    assert(it != functions_.end());
    it->second.increment_parameter_evaluation(formal_parameter_position);

    for (auto it = call_stack_.rbegin(); it != call_stack_.rend(); ++it) {
        if (it->call_id == call_id) {
            it->update_formal_parameter_usage_order(formal_parameter_position);
            break;
        }
    }
}

void StrictnessAnalysis::push_on_call_stack(CallState call_state) {
    call_stack_.push_back(call_state);
}

CallState StrictnessAnalysis::pop_from_call_stack(call_id_t call_id) {
    CallState call_state = call_stack_.back();
    call_stack_.pop_back();
    assert(call_state.call_id == call_id);
    return call_state;
}

void StrictnessAnalysis::serialize() {

    for (const auto &key_value : promises_) {
        compute_evaluation_distance(nullptr, key_value.second);
    }

    serialize_function_formal_parameter_usage_count();
    serialize_function_formal_parameter_usage_order();
    serialize_function_call_count();
    serialize_evaluation_distance();
    serialize_evaluation_context_counts();
}

void StrictnessAnalysis::serialize_function_formal_parameter_usage_count() {
    std::ofstream fout(output_dir_ +
                           "/function-formal-parameter-usage-count.csv",
                       std::ios::trunc);
    fout << "function_id"
         << " , "
         << "formal_parameter_position"
         << " , "
         << "usage"
         << " , "
         << "count" << std::endl;

    for (auto const &pair : functions_) {
        fn_id_t fn_id = pair.first;
        std::vector<int> parameter_evaluation =
            pair.second.parameter_evaluation;
        std::vector<int> parameter_metaprogramming =
            pair.second.parameter_metaprogramming;
        for (size_t i = 0; i < parameter_evaluation.size(); ++i) {
            fout << fn_id << " , " << i << " , "
                 << "E"
                 << " , " << parameter_evaluation[i] << std::endl;
            fout << fn_id << " , " << i << " , "
                 << "M"
                 << " , " << parameter_metaprogramming[i] << std::endl;
        }
    }

    fout.close();
}

void StrictnessAnalysis::serialize_function_formal_parameter_usage_order() {
    std::ofstream fout(output_dir_ +
                           "/function-formal-parameter-usage-order.csv",
                       std::ios::trunc);
    fout << "function_id"
         << " , "
         << "formal_parameter_position_usage_order"
         << " , "
         << "count" << std::endl;

    for (auto const &pair : functions_) {
        fn_id_t fn_id = pair.first;
        std::vector<std::string> parameter_usage_order =
            pair.second.parameter_usage_order;
        std::vector<int> parameter_usage_order_count =
            pair.second.parameter_usage_order_count;
        for (size_t i = 0; i < parameter_usage_order.size(); ++i) {
            fout << fn_id << " , " << parameter_usage_order[i] << " , "
                 << parameter_usage_order_count[i] << std::endl;
        }
    }

    fout.close();
}

void StrictnessAnalysis::serialize_function_call_count() {
    std::ofstream fout(output_dir_ + "/function-call-count.csv",
                       std::ios::trunc);
    fout << "function_id"
         << " , "
         << "count" << std::endl;

    for (auto const &pair : functions_) {
        fn_id_t fn_id = pair.first;
        int call_count = pair.second.call_count;
        fout << fn_id << " , " << call_count << std::endl;
    }

    fout.close();
}

void StrictnessAnalysis::serialize_evaluation_distance() {
    std::ofstream fout(output_dir_ + "/promise-evaluation-distance.csv",
                       std::ios::trunc);

    fout << "closure_count"
         << " , "
         << "special_count"
         << " , "
         << "builtin_count"
         << " , "
         << "promise_count"
         << " , "
         << "argument_category"
         << " , "
         << "evaluated"
         << " , "
         << "count" << std::endl;

    for (const auto &key_value : evaluation_distances_) {
        fout << key_value.first << " , " << key_value.second << std::endl;
    }

    fout.close();
}

void StrictnessAnalysis::serialize_evaluation_context_counts() {
    std::ofstream fout(output_dir_ + "/promise-evaluation-context.csv",
                       std::ios::trunc);
    fout << "context"
         << " , "
         << "promise_count" << std::endl;

    for (int i = 0; i < evaluation_context_counts_.size(); ++i) {
        fout << evaluation_contexts[i] << " , " << evaluation_context_counts_[i]
             << std::endl;
    }

    fout.close();
}
