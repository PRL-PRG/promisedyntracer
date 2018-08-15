#include "StrictnessAnalysis.h"

const size_t FUNCTION_MAPPING_BUCKET_SIZE = 20000;

StrictnessAnalysis::StrictnessAnalysis(const tracer_state_t &tracer_state,
                                       PromiseMapper *const promise_mapper,
                                       const std::string &output_dir,
                                       bool truncate, bool binary,
                                       int compression_level)
    : tracer_state_(tracer_state), output_dir_(output_dir),
      promise_mapper_(promise_mapper),
      functions_(std::unordered_map<fn_id_t, FunctionState>(
          FUNCTION_MAPPING_BUCKET_SIZE)) {

    usage_data_table_ = create_data_table(
        output_dir + "/" + "parameter-usage-count",
        {"function_id", "call_id", "position", "default", "unpromised", "force",
         "lookup", "metaprogram", "metaprogram_and_lookup", "use"},
        truncate, binary, compression_level);

    order_data_table_ = create_data_table(
        output_dir + "/" + "parameter-force-order",
        {"function_id", "order", "count"}, truncate, binary, compression_level);
}

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
        /* Process arguments that have been unpromised. */
        if (argument.value_type != PROMSXP) {
            call_stack_.back().unpromise(argument.formal_parameter_position,
                                         argument.default_argument);
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

    it->second.add_order(call_state.get_order());
}

void StrictnessAnalysis::push_on_call_stack(CallState call_state) {
    call_stack_.push_back(call_state);
}

CallState StrictnessAnalysis::pop_from_call_stack(call_id_t call_id) {
    CallState call_state = call_stack_.back();
    call_stack_.pop_back();

    if (call_state.get_call_id() != call_id) {
        std::cerr << "call " << call_id << " does not match "
                  << call_state.get_call_id() << " on stack." << std::endl;
        exit(EXIT_FAILURE);
    }

    serialize_parameter_usage_count(call_state);
    return call_state;
}

void StrictnessAnalysis::end(dyntracer_t *dyntracer) { serialize(); }

StrictnessAnalysis::~StrictnessAnalysis() {
    delete usage_data_table_;
    delete order_data_table_;
}

void StrictnessAnalysis::serialize() { serialize_parameter_usage_order(); }

void StrictnessAnalysis::serialize_parameter_usage_count(
    const CallState &call_state) {

    const auto &default_parameter_uses{call_state.get_default_parameter_uses()};
    const auto &custom_parameter_uses{call_state.get_custom_parameter_uses()};

    for (int position = 0; position < call_state.get_formal_parameter_count();
         ++position) {

        const auto &default_parameter{default_parameter_uses[position]};
        const auto &custom_parameter{custom_parameter_uses[position]};

        usage_data_table_->write_row(
            call_state.get_function_id(),
            static_cast<double>(call_state.get_call_id()), position, "DA",
            default_parameter.get_unpromised(), default_parameter.get_forced(),
            default_parameter.get_looked_up(),
            default_parameter.get_metaprogrammed(),
            default_parameter.get_metaprogrammed_and_looked_up(),
            default_parameter.get_used());

        usage_data_table_->write_row(
            call_state.get_function_id(),
            static_cast<double>(call_state.get_call_id()), position, "CA",
            custom_parameter.get_unpromised(), custom_parameter.get_forced(),
            custom_parameter.get_looked_up(),
            custom_parameter.get_metaprogrammed(),
            custom_parameter.get_metaprogrammed_and_looked_up(),
            custom_parameter.get_used());
    }
}

void StrictnessAnalysis::serialize_parameter_usage_order() {
    for (auto const &pair : functions_) {
        fn_id_t fn_id = pair.first;
        const auto &orders_{pair.second.get_orders()};
        const auto &order_counts_{pair.second.get_order_counts()};
        for (std::size_t i = 0; i < orders_.size(); ++i) {
            order_data_table_->write_row(fn_id, orders_[i],
                                         (double)order_counts_[i]);
        }
    }
}

void StrictnessAnalysis::promise_force_entry(const prom_info_t &prom_info,
                                             const SEXP promise) {
    PromiseState &promise_state{promise_mapper_->find(prom_info.prom_id)};

    /* if promise is not an argument, then don't process it. */
    if (!promise_state.argument) {
        return;
    }

    auto *call_state = get_call_state(promise_state.call_id);

    /* this implies an escaped promise, the promise is alive but not
       the function invocation that got this promise as its argument */
    if (call_state == nullptr) {
        return;
    }

    call_state->force(promise_state.formal_parameter_position,
                      promise_state.default_argument);
}

void StrictnessAnalysis::promise_value_lookup(const prom_info_t &prom_info,
                                              const SEXP promise) {
    PromiseState &promise_state{promise_mapper_->find(prom_info.prom_id)};

    /* if promise is not an argument, then don't process it. */
    if (!promise_state.argument) {
        return;
    }

    auto *call_state = get_call_state(promise_state.call_id);

    /* this implies an escaped promise, the promise is alive but not
       the function invocation that got this promise as its argument */
    if (call_state == nullptr) {
        return;
    }

    call_state->lookup(promise_state.formal_parameter_position,
                       promise_state.default_argument);
}

void StrictnessAnalysis::promise_value_assign(const prom_info_t &prom_info,
                                              const SEXP promise) {
    metaprogram_(prom_info, promise);
}

void StrictnessAnalysis::promise_environment_lookup(
    const prom_info_t &prom_info, const SEXP promise) {
    metaprogram_(prom_info, promise);
}
void StrictnessAnalysis::promise_environment_assign(
    const prom_info_t &prom_info, const SEXP promise) {
    metaprogram_(prom_info, promise);
}
void StrictnessAnalysis::promise_expression_lookup(const prom_info_t &prom_info,
                                                   const SEXP promise) {
    metaprogram_(prom_info, promise);
}
void StrictnessAnalysis::promise_expression_assign(const prom_info_t &prom_info,
                                                   const SEXP promise) {
    metaprogram_(prom_info, promise);
}

void StrictnessAnalysis::metaprogram_(const prom_info_t &prom_info,
                                      const SEXP promise) {
    PromiseState &promise_state{promise_mapper_->find(prom_info.prom_id)};

    /* if promise is not an argument, then don't process it. */
    if (!promise_state.argument) {
        return;
    }

    auto *call_state = get_call_state(promise_state.call_id);

    /* this implies an escaped promise, the promise is alive but not
       the function invocation that got this promise as its argument */
    if (call_state == nullptr) {
        return;
    }

    call_state->metaprogram(promise_state.formal_parameter_position,
                            promise_state.default_argument);
}

CallState *StrictnessAnalysis::get_call_state(const call_id_t call_id) {

    auto it{call_stack_.rbegin()};

    for (; it != call_stack_.rend(); ++it) {
        if (it->get_call_id() == call_id) {
            break;
        }
    }

    if (it == call_stack_.rend()) {
        return nullptr;
    }

    return &(*it);
}
