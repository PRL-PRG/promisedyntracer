#include "StrictnessAnalysis.h"

const size_t FUNCTION_MAPPING_BUCKET_SIZE = 20000;

StrictnessAnalysis::StrictnessAnalysis(const tracer_state_t &tracer_state,
                                       const std::string &output_dir,
                                       PromiseMapper *const promise_mapper)
    : tracer_state_(tracer_state), output_dir_(output_dir),
      promise_mapper_(promise_mapper),
      functions_(std::unordered_map<fn_id_t, FunctionState>(
          FUNCTION_MAPPING_BUCKET_SIZE)),
      position_table_writer_{output_dir + "/" + "parameter-usage-count.csv",
                             {"function_id", "formal_parameter_position",
                              "default", "use", "count"}},
      order_table_writer_{
          output_dir + "/" + "parameter-usage-order.csv",
          {"function_id", "formal_parameter_position_usage_order", "count"}} {}

/* When we enter a function, push information about it on a custom call stack.
   We also update the function table to create an entry for this function.
   This entry contains the evaluation information of the function's arguments.
 */
void StrictnessAnalysis::closure_entry(const closure_info_t &closure_info) {
    // push call_id to call_stack
    call_id_t call_id = closure_info.call_id;
    fn_id_t fn_id = closure_info.fn_id;

    push_on_call_stack(
        CallState(call_id, fn_id, closure_info.formal_parameter_count));

    auto fn_iter = functions_.insert(std::make_pair(
        fn_id, FunctionState(closure_info.formal_parameter_count)));
    fn_iter.first->second.increment_call();

    for (const auto &argument : closure_info.arguments) {
        /* Process arguments that have been unpromised.
           Assume that unpromised arguments are forced.
           Add them to usage order and increment the
           corresponding formal parameter position.
        */
        if (argument.value_type != PROMSXP) {
            call_stack_.back().add_use(argument.formal_parameter_position,
                                       argument.default_argument,
                                       PromiseUse::Force);
        }
    }
}

void StrictnessAnalysis::closure_exit(const closure_info_t &closure_info) {
    remove_stack_frame(closure_info.call_id, closure_info.fn_id);
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

/* We remove the call information from the call stack.
   The call information tells us the usage order of function arguments.
   This usage order is stored in the function object corresponding to
   this call. The count for this order is also incremented.
 */
void StrictnessAnalysis::remove_stack_frame(call_id_t call_id, fn_id_t fn_id) {
    // pop call_id from call_stack
    CallState call_state = pop_from_call_stack(call_id);
    auto it = functions_.find(fn_id);
    if (it == functions_.end()) {
        std::cerr << "Function: " << fn_id << " and "
                  << "call " << call_id << " not found." << std::endl;
        exit(EXIT_FAILURE);
    }
    it->second.add_uses(call_state.get_uses());
    it->second.add_order(call_state.get_order());
}

void StrictnessAnalysis::push_on_call_stack(CallState call_state) {
    call_stack_.push_back(call_state);
}

CallState StrictnessAnalysis::pop_from_call_stack(call_id_t call_id) {
    CallState call_state = call_stack_.back();
    call_stack_.pop_back();

    if (call_state.call_id != call_id) {
        std::cerr << "call " << call_id << " does not match "
                  << call_state.call_id << " on stack." << std::endl;
        exit(EXIT_FAILURE);
    }

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
        const auto &uses{pair.second.get_uses()};
        for (std::size_t parameter = 0; parameter < uses.size(); ++parameter) {
            for (std::size_t type = 0; type < 2; ++type) {
                for (std::size_t use = 0;
                     use < to_underlying_type(PromiseUse::Count); ++use) {
                    position_table_writer_.write_row(
                        fn_id, parameter, type,
                        to_string(static_cast<PromiseUse>(use)),
                        uses[parameter][type][use]);
                }
            }
        }
    }
}

void StrictnessAnalysis::serialize_function_formal_parameter_usage_order() {
    for (auto const &pair : functions_) {
        fn_id_t fn_id = pair.first;
        const auto &orders_{pair.second.get_orders()};
        const auto &order_counts_{pair.second.get_order_counts()};
        for (std::size_t i = 0; i < orders_.size(); ++i) {
            order_table_writer_.write_row(fn_id, orders_[i], order_counts_[i]);
        }
    }
}

void StrictnessAnalysis::promise_force_entry(const prom_info_t &prom_info,
                                             const SEXP promise) {
    _update_argument_usage(prom_info, promise, PromiseUse::Force);
}

void StrictnessAnalysis::promise_value_lookup(const prom_info_t &prom_info,
                                              const SEXP promise) {
    _update_argument_usage(prom_info, promise, PromiseUse::ValueLookup);
}

void StrictnessAnalysis::promise_value_assign(const prom_info_t &prom_info,
                                              const SEXP promise) {
    _update_argument_usage(prom_info, promise, PromiseUse::ValueAssign);
}

void StrictnessAnalysis::promise_environment_lookup(
    const prom_info_t &prom_info, const SEXP promise) {
    _update_argument_usage(prom_info, promise, PromiseUse::EnvironmentLookup);
}
void StrictnessAnalysis::promise_environment_assign(
    const prom_info_t &prom_info, const SEXP promise) {
    _update_argument_usage(prom_info, promise, PromiseUse::EnvironmentAssign);
}
void StrictnessAnalysis::promise_expression_lookup(const prom_info_t &prom_info,
                                                   const SEXP promise) {
    _update_argument_usage(prom_info, promise, PromiseUse::ExpressionLookup);
}
void StrictnessAnalysis::promise_expression_assign(const prom_info_t &prom_info,
                                                   const SEXP promise) {
    _update_argument_usage(prom_info, promise, PromiseUse::ExpressionAssign);
}

void StrictnessAnalysis::_update_argument_usage(const prom_info_t &prom_info,
                                                const SEXP promise,
                                                PromiseUse use) {

    PromiseState &promise_state{promise_mapper_->find(prom_info.prom_id)};

    /* if promise is not an argument, then don't process it. */
    if (!promise_state.argument) {
        return;
    }

    call_id_t call_id = promise_state.call_id;

    auto it{call_stack_.rbegin()};

    for (; it != call_stack_.rend(); ++it) {
        if (it->call_id == call_id) {
            break;
        }
    }

    /* this implies an escaped promise, the promise is alive but not
       the function invocation that got this promise as its argument */
    if (it == call_stack_.rend()) {
        return;
    }

    it->add_use(promise_state.formal_parameter_position,
                promise_state.default_argument, use);
}
