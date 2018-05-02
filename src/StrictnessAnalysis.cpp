#include "StrictnessAnalysis.h"

const size_t FUNCTION_MAPPING_BUCKET_SIZE = 20000;
const size_t PROMISE_MAPPING_BUCKET_SIZE = 20000000;

StrictnessAnalysis::StrictnessAnalysis(const tracer_state_t &tracer_state,
                                       const std::string &output_dir)
    : tracer_state_(tracer_state), output_dir_(output_dir),
      functions_(std::unordered_map<fn_id_t, FunctionState>(
          FUNCTION_MAPPING_BUCKET_SIZE)),
      promises_(std::unordered_map<prom_id_t, ArgumentPromiseState>(
          PROMISE_MAPPING_BUCKET_SIZE)) {}

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
                                  actual_argument_position)});
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
    call_id_t call_id = iter->second.call_id;
    if (!is_executing(call_id))
        return;
    fn_id_t fn_id = iter->second.fn_id;
    int formal_parameter_position = iter->second.formal_parameter_position;

    update_argument_position(call_id, fn_id, formal_parameter_position);
}

void StrictnessAnalysis::promise_force_exit(const prom_info_t &prom_info,
                                            const SEXP promise) {
    // TODO - remove promise from map
}

void StrictnessAnalysis::gc_promise_unmarked(const prom_id_t &prom_id,
                                             const SEXP promise) {
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

    serialize_function_formal_parameter_usage_count();
    serialize_function_formal_parameter_usage_order();
    serialize_function_call_count();
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
