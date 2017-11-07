#include "probes.h"

void begin(dyntrace_context_t *context, const SEXP prom) {
    tracer_state(context).start_pass(context, prom);
    metadata_t metadata;
    get_environment_metadata(metadata);
    get_current_time_metadata(metadata, "START");
    tracer_serializer(context).serialize_start_trace(metadata);
}

void end(dyntrace_context_t *context) {
    tracer_state(context).finish_pass();

    metadata_t metadata;
    get_current_time_metadata(metadata, "END");
    tracer_serializer(context).serialize_finish_trace(metadata);

    if (!tracer_state(context).fun_stack.empty()) {
        Rprintf("Function stack is not balanced: %d remaining.\n",
                tracer_state(context).fun_stack.size());
        tracer_state(context).fun_stack.clear();
    }

    if (!tracer_state(context).full_stack.empty()) {
        Rprintf("Function/promise stack is not balanced: %d remaining.\n",
                tracer_state(context).full_stack.size());
        tracer_state(context).full_stack.clear();
    }
}

// Triggered when entering function evaluation.
void function_entry(dyntrace_context_t *context, const SEXP call, const SEXP op,
                    const SEXP rho) {
    closure_info_t info = function_entry_get_info(context, call, op, rho);

    // Push function ID on function stack
    tracer_state(context).fun_stack.push_back(
        make_tuple(info.call_id, info.fn_id, info.fn_type));
    tracer_state(context).curr_env_stack.push(info.call_ptr);

    tracer_serializer(context).serialize_function_entry(context, info);

    auto &fresh_promises = tracer_state(context).fresh_promises;
    // Associate promises with call ID
    for (auto arg_ref : info.arguments.all()) {
        const arg_t &argument = arg_ref.get();
        auto &promise = get<2>(argument);
        auto it = fresh_promises.find(promise);
        // if promise environment is same as the caller's environment, then
        // serialize this promise as it is a default argument.

        tracer_serializer(context).serialize_promise_argument_type(
            promise, get<3>(argument));

        if (it != fresh_promises.end()) {
            tracer_state(context).promise_origin[promise] = info.call_id;
            fresh_promises.erase(it);
        }
    }
}

void function_exit(dyntrace_context_t *context, const SEXP call, const SEXP op,
                   const SEXP rho, const SEXP retval) {
    closure_info_t info = function_exit_get_info(context, call, op, rho);
    tracer_serializer(context).serialize_function_exit(info);

    // Current function ID is popped in function_exit_get_info
    tracer_state(context).curr_env_stack.pop();
}

void print_entry_info(dyntrace_context_t *context, const SEXP call,
                      const SEXP op, const SEXP rho, function_type fn_type) {
    builtin_info_t info =
        builtin_entry_get_info(context, call, op, rho, fn_type);
    tracer_serializer(context).serialize_builtin_entry(context, info);

    tracer_state(context).fun_stack.push_back(
        make_tuple(info.call_id, info.fn_id, info.fn_type));
    tracer_state(context).curr_env_stack.push(info.call_ptr | 1);
}

void builtin_entry(dyntrace_context_t *context, const SEXP call, const SEXP op,
                   const SEXP rho) {
    function_type fn_type;
    if (TYPEOF(op) == BUILTINSXP)
        fn_type = (PRIMINTERNAL(op) == 0) ? function_type::TRUE_BUILTIN
                                          : function_type::BUILTIN;
    else /*the weird case of NewBuiltin2 , where op is a language expression*/
        fn_type = function_type::TRUE_BUILTIN;
    print_entry_info(context, call, op, rho, fn_type);
}

void specialsxp_entry(dyntrace_context_t *context, const SEXP call,
                      const SEXP op, const SEXP rho) {
    print_entry_info(context, call, op, rho, function_type::SPECIAL);
}

void print_exit_info(dyntrace_context_t *context, const SEXP call,
                     const SEXP op, const SEXP rho, function_type fn_type) {
    builtin_info_t info =
        builtin_exit_get_info(context, call, op, rho, fn_type);
    tracer_serializer(context).serialize_builtin_exit(info);

    tracer_state(context).fun_stack.pop_back();
    tracer_state(context).curr_env_stack.pop();
}

void builtin_exit(dyntrace_context_t *context, const SEXP call, const SEXP op,
                  const SEXP rho, const SEXP retval) {
    function_type fn_type;
    if (TYPEOF(op) == BUILTINSXP)
        fn_type = (PRIMINTERNAL(op) == 0) ? function_type::TRUE_BUILTIN
                                          : function_type::BUILTIN;
    else
        fn_type = function_type::TRUE_BUILTIN;
    print_exit_info(context, call, op, rho, fn_type);
}

void specialsxp_exit(dyntrace_context_t *context, const SEXP call,
                     const SEXP op, const SEXP rho, const SEXP retval) {
    print_exit_info(context, call, op, rho, function_type::SPECIAL);
}

void promise_created(dyntrace_context_t *context, const SEXP prom,
                     const SEXP rho) {
    prom_basic_info_t info = create_promise_get_info(context, prom, rho);
    tracer_serializer(context).serialize_promise_created(info);
    if (info.prom_id >= 0) { // maybe we don't need this check
        tracer_serializer(context).serialize_promise_lifecycle(
            {info.prom_id, 0, tracer_state(context).gc_trigger_counter});
    }
}

// Promise is being used inside a function body for the first time.
void promise_force_entry(dyntrace_context_t *context, const SEXP symbol,
                         const SEXP rho) {
    prom_info_t info = force_promise_entry_get_info(context, symbol, rho);
    tracer_serializer(context).serialize_force_promise_entry(
        context, info, tracer_state(context).clock_id);
    tracer_state(context).clock_id++;
    if (info.prom_id >= 0) {
        tracer_serializer(context).serialize_promise_lifecycle(
            {info.prom_id, 1, tracer_state(context).gc_trigger_counter});
    }
}

void promise_force_exit(dyntrace_context_t *context, const SEXP symbol,
                        const SEXP rho, const SEXP val) {
    prom_info_t info = force_promise_exit_get_info(context, symbol, rho, val);
    tracer_serializer(context).serialize_force_promise_exit(
        info, tracer_state(context).clock_id);
    tracer_state(context).clock_id++;
}

void promise_value_lookup(dyntrace_context_t *context, const SEXP symbol,
                          const SEXP rho, const SEXP val) {
    prom_info_t info = promise_lookup_get_info(context, symbol, rho, val);
    if (info.prom_id >= 0) {
        tracer_serializer(context).serialize_promise_lookup(
            info, tracer_state(context).clock_id);
        tracer_state(context).clock_id++;
        tracer_serializer(context).serialize_promise_lifecycle(
            {info.prom_id, 1, tracer_state(context).gc_trigger_counter});
    }
}

void promise_expression_lookup(dyntrace_context_t *context, const SEXP prom) {

    prom_info_t info = promise_expression_lookup_get_info(context, prom);
    if (info.prom_id >= 0) {
        tracer_serializer(context).serialize_promise_expression_lookup(
            info, tracer_state(context).clock_id);
        tracer_state(context).clock_id++;
        tracer_serializer(context).serialize_promise_lifecycle(
            {info.prom_id, 3, tracer_state(context).gc_trigger_counter});
    }
}

void gc_promise_unmarked(dyntrace_context_t *context, const SEXP promise) {
    prom_addr_t addr = get_sexp_address(promise);
    prom_id_t id = get_promise_id(context, promise);
    auto &promise_origin = tracer_state(context).promise_origin;

    if (id >= 0) {
        tracer_serializer(context).serialize_promise_lifecycle(
            {id, 2, tracer_state(context).gc_trigger_counter});
    }

    auto iter = promise_origin.find(id);
    if (iter != promise_origin.end()) {
        // If this is one of our traced promises,
        // delete it from origin map because it is ready to be GCed
        promise_origin.erase(iter);
        // Rprintf("Promise %#x deleted.\n", id);
    }

    unsigned int prom_type = TYPEOF(PRCODE(promise));
    unsigned int orig_type =
        (prom_type == 21) ? TYPEOF(BODY_EXPR(PRCODE(promise))) : 0;
    prom_key_t key(addr, prom_type, orig_type);
    tracer_state(context).promise_ids.erase(key);
}

void gc_entry(dyntrace_context_t *context, R_size_t size_needed) {
    tracer_state(context).gc_trigger_counter =
        1 + tracer_state(context).gc_trigger_counter;
}

void gc_exit(dyntrace_context_t *context, int gc_count, double vcells,
             double ncells) {
    gc_info_t info = gc_exit_get_info(gc_count, vcells, ncells);
    info.counter = tracer_state(context).gc_trigger_counter;
    tracer_serializer(context).serialize_gc_exit(info);
}

void vector_alloc(dyntrace_context_t *context, int sexptype, long length,
                  long bytes, const char *srcref) {
    type_gc_info_t info{tracer_state(context).gc_trigger_counter, sexptype,
                        length, bytes};
    tracer_serializer(context).serialize_vector_alloc(info);
}

void new_environment(dyntrace_context_t *context, const SEXP rho) {
    fn_id_t fn_id = std::get<fn_id_t>(tracer_state(context).fun_stack.back());
    env_id_t env_id = tracer_state(context).environment_id_counter++;
    tracer_serializer(context).serialize_new_environment(env_id, fn_id);
}

void jump_ctxt(dyntrace_context_t *context, const SEXP rho, const SEXP val) {
    vector<call_id_t> unwound_calls;
    vector<prom_id_t> unwound_promises;
    unwind_info_t info;

    tracer_state(context).adjust_stacks(rho, info);

    tracer_serializer(context).serialize_unwind(info);
}
