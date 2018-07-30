#include "StrictnessAnalysis.h"

const size_t FUNCTION_MAPPING_BUCKET_SIZE = 20000;

StrictnessAnalysis::StrictnessAnalysis(const tracer_state_t &tracer_state,
                                       const std::string &output_dir,
                                       PromiseMapper *const promise_mapper)
    : tracer_state_(tracer_state), output_dir_(output_dir),
      promise_mapper_(promise_mapper),
      functions_(std::unordered_map<fn_id_t, FunctionState>(
          FUNCTION_MAPPING_BUCKET_SIZE)),
      position_table_writer_{
          output_dir + "/" + "function-formal-parameter-usage-count.csv",
          {"function_id", "formal_parameter_position", "count"}},
      order_table_writer_{
          output_dir + "/" + "function-formal-parameter-usage-order.csv",
          {"function_id", "formal_parameter_position_usage_order", "count"}} {}

/* When we enter a function, push information about it on a custom call stack.
   We also update the function table to create an entry for this function.
   This entry contains the evaluation information of the function's arguments.
 */
void StrictnessAnalysis::closure_entry(const closure_info_t &closure_info) {
    // push call_id to call_stack
    call_id_t call_id = closure_info.call_id;
    fn_id_t fn_id = closure_info.fn_id;

    push_on_call_stack(CallState(call_id, fn_id));

    // max_position will contain the number of formal parameters, the
    // function expects.
    int max_position = 0;

    for (const auto &argument : closure_info.arguments) {
        int formal_parameter_position = argument.formal_parameter_position;
        // int actual_argument_position = argument.actual_argument_position;
        if (formal_parameter_position > max_position)
            max_position = formal_parameter_position;
    }

    auto fn_iter = functions_.insert(
        std::make_pair(fn_id, FunctionState(max_position + 1)));
    fn_iter.first->second.increment_call();
}

void StrictnessAnalysis::closure_exit(const closure_info_t &closure_info) {
    remove_stack_frame(closure_info.call_id, closure_info.fn_id);
}

void StrictnessAnalysis::promise_force_entry(const prom_info_t &prom_info,
                                             const SEXP promise) {
    prom_id_t prom_id = prom_info.prom_id;
    PromiseState &promise_state = promise_mapper_->find(prom_id);
    if (promise_state.local && promise_state.argument) {
        call_id_t call_id = promise_state.call_id;
        if (!is_executing(call_id))
            return;
        fn_id_t fn_id = promise_state.fn_id;
        int formal_parameter_position = promise_state.formal_parameter_position;
        update_argument_position(call_id, fn_id, formal_parameter_position);
    }
}

void StrictnessAnalysis::gc_promise_unmarked(const prom_id_t prom_id,
                                             const SEXP promise) {
    PromiseState &promise_state = promise_mapper_->find(prom_id);
}

void StrictnessAnalysis::context_jump(const unwind_info_t &info) {
    for (auto &element : info.unwound_frames) {
        if (element.type == stack_type::CALL &&
            element.function_info.type == function_type::CLOSURE) {
            remove_stack_frame(element.call_id,
                               element.function_info.function_id);
        }
    }
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
    // std::cout << std::endl
    //           << fn_id << " , " << call_id << " , "
    //           << formal_parameter_position;

    FunctionState function_state = it->second;
    function_state.increment_parameter_evaluation(formal_parameter_position);
    it->second = function_state;

    for (auto it = call_stack_.rbegin(); it != call_stack_.rend(); ++it) {
        if (it->call_id == call_id) {
            it->update_formal_parameter_usage_order(formal_parameter_position);
            break;
        }
    }
}

/* We remove the call information from the call stack.
   The call information tells us the usage order of function arguments.
   This usage order is stored in the function object corresponding to
   this call. The count for this order is also incremented.
 */
void StrictnessAnalysis::remove_stack_frame(call_id_t call_id, fn_id_t fn_id) {
    // pop call_id from call_stack
    CallState call_state = pop_from_call_stack(call_id);
    auto it = functions_.find(fn_id);
    assert(it != functions_.end());
    it->second.add_formal_parameter_usage_order(
        call_state.formal_parameter_usage_order);
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

void StrictnessAnalysis::end(dyntracer_t *dyntracer) { serialize(); }

void StrictnessAnalysis::serialize() {

    serialize_function_formal_parameter_usage_count();
    serialize_function_formal_parameter_usage_order();
}

void StrictnessAnalysis::serialize_function_formal_parameter_usage_count() {
    for (auto const &pair : functions_) {
        fn_id_t fn_id = pair.first;
        std::vector<int> parameter_evaluation =
            pair.second.parameter_evaluation;
        for (size_t i = 0; i < parameter_evaluation.size(); ++i) {
            position_table_writer_.write_row(fn_id, i, parameter_evaluation[i]);
        }
    }
}

void StrictnessAnalysis::serialize_function_formal_parameter_usage_order() {
    for (auto const &pair : functions_) {
        fn_id_t fn_id = pair.first;
        std::vector<std::string> parameter_usage_order =
            pair.second.parameter_usage_order;
        std::vector<int> parameter_usage_order_count =
            pair.second.parameter_usage_order_count;
        for (size_t i = 0; i < parameter_usage_order.size(); ++i) {
            order_table_writer_.write_row(fn_id, parameter_usage_order[i],
                                          parameter_usage_order_count[i]);
        }
    }
}
