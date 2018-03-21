#include "probes.h"
#include "State.h"

void begin(dyntrace_context_t *context, const SEXP prom) {
    tracer_state(context).start_pass(context, prom);
    debug_serializer(context).serialize_start_trace();
    tracer_serializer(context).serialize_start_trace();
    environment_variables_t environment_variables =
        context->dyntracing_context->environment_variables;
    tracer_serializer(context).serialize_metadatum("GIT_COMMIT_INFO",
                                                   GIT_COMMIT_INFO);
    tracer_serializer(context).serialize_metadatum(
        "DYNTRACE_BEGIN_DATETIME",
        remove_null(context->dyntracing_context->begin_datetime));
    tracer_serializer(context).serialize_metadatum(
        "R_COMPILE_PKGS", remove_null(environment_variables.r_compile_pkgs));
    tracer_serializer(context).serialize_metadatum(
        "R_DISABLE_BYTECODE",
        remove_null(environment_variables.r_disable_bytecode));
    tracer_serializer(context).serialize_metadatum(
        "R_ENABLE_JIT", remove_null(environment_variables.r_enable_jit));
    tracer_serializer(context).serialize_metadatum(
        "R_KEEP_PKG_SOURCE",
        remove_null(environment_variables.r_keep_pkg_source));
    tracer_serializer(context).serialize_metadatum(
        "RDT_COMPILE_VIGNETTE", remove_null(getenv("RDT_COMPILE_VIGNETTE")));
}

void serialize_execution_time(dyntrace_context_t *context) {
    SqlSerializer &serializer = tracer_serializer(context);
    execution_time_t execution_time =
        context->dyntracing_context->execution_time;
    serializer.serialize_metadatum(
        "PROBE_FUNCTION_ENTRY",
        clock_ticks_to_string(execution_time.probe_function_entry));
    serializer.serialize_metadatum(
        "PROBE_FUNCTION_EXIT",
        clock_ticks_to_string(execution_time.probe_function_exit));
    serializer.serialize_metadatum(
        "PROBE_BUILTIN_ENTRY",
        clock_ticks_to_string(execution_time.probe_builtin_entry));
    serializer.serialize_metadatum(
        "PROBE_BUILTIN_EXIT",
        clock_ticks_to_string(execution_time.probe_builtin_exit));
    serializer.serialize_metadatum(
        "PROBE_SPECIALSXP_ENTRY",
        clock_ticks_to_string(execution_time.probe_specialsxp_entry));
    serializer.serialize_metadatum(
        "PROBE_SPECIALSXP_EXIT",
        clock_ticks_to_string(execution_time.probe_specialsxp_exit));
    serializer.serialize_metadatum(
        "PROBE_PROMISE_CREATED",
        clock_ticks_to_string(execution_time.probe_promise_created));
    serializer.serialize_metadatum(
        "PROBE_PROMISE_FORCE_ENTRY",
        clock_ticks_to_string(execution_time.probe_promise_force_entry));
    serializer.serialize_metadatum(
        "PROBE_PROMISE_FORCE_EXIT",
        clock_ticks_to_string(execution_time.probe_promise_force_exit));
    serializer.serialize_metadatum(
        "PROBE_PROMISE_VALUE_LOOKUP",
        clock_ticks_to_string(execution_time.probe_promise_value_lookup));
    serializer.serialize_metadatum(
        "PROBE_PROMISE_EXPRESSION_LOOKUP",
        clock_ticks_to_string(execution_time.probe_promise_expression_lookup));
    serializer.serialize_metadatum(
        "PROBE_ERROR", clock_ticks_to_string(execution_time.probe_error));
    serializer.serialize_metadatum(
        "PROBE_VECTOR_ALLOC",
        clock_ticks_to_string(execution_time.probe_vector_alloc));
    serializer.serialize_metadatum(
        "PROBE_EVAL_ENTRY",
        clock_ticks_to_string(execution_time.probe_eval_entry));
    serializer.serialize_metadatum(
        "PROBE_EVAL_EXIT",
        clock_ticks_to_string(execution_time.probe_eval_exit));
    serializer.serialize_metadatum(
        "PROBE_GC_ENTRY", clock_ticks_to_string(execution_time.probe_gc_entry));
    serializer.serialize_metadatum(
        "PROBE_GC_EXIT", clock_ticks_to_string(execution_time.probe_gc_exit));
    serializer.serialize_metadatum(
        "PROBE_GC_PROMISE_UNMARKED",
        clock_ticks_to_string(execution_time.probe_gc_promise_unmarked));
    serializer.serialize_metadatum(
        "PROBE_JUMP_CTXT",
        clock_ticks_to_string(execution_time.probe_jump_ctxt));
    serializer.serialize_metadatum(
        "PROBE_NEW_ENVIRONMENT",
        clock_ticks_to_string(execution_time.probe_new_environment));
    serializer.serialize_metadatum(
        "PROBE_S3_GENERIC_ENTRY",
        clock_ticks_to_string(execution_time.probe_S3_generic_entry));
    serializer.serialize_metadatum(
        "PROBE_S3_GENERIC_EXIT",
        clock_ticks_to_string(execution_time.probe_S3_generic_exit));
    serializer.serialize_metadatum(
        "PROBE_S3_DISPATCH_ENTRY",
        clock_ticks_to_string(execution_time.probe_S3_dispatch_entry));
    serializer.serialize_metadatum(
        "PROBE_S3_DISPATCH_EXIT",
        clock_ticks_to_string(execution_time.probe_S3_dispatch_exit));
    serializer.serialize_metadatum(
        "EXPRESSION", clock_ticks_to_string(execution_time.expression));
}

void end(dyntrace_context_t *context) {
    tracer_state(context).finish_pass();
    serialize_execution_time(context);
    // serialize_execution_count(context);
    debug_serializer(context).serialize_finish_trace();
    tracer_serializer(context).serialize_finish_trace();
    tracer_serializer(context).serialize_metadatum(
        "DYNTRACE_END_DATETIME",
        remove_null(context->dyntracing_context->end_datetime));

//    if (!tracer_state(context).fun_stack.empty()) {
//        dyntrace_log_warning("Function stack is not balanced: %d remaining",
//                             tracer_state(context).fun_stack.size());
//        tracer_state(context).fun_stack.clear();
//    }

    if (!tracer_state(context).full_stack.empty()) {
        dyntrace_log_warning(
            "Function/promise stack is not balanced: %d remaining",
            tracer_state(context).full_stack.size());
        tracer_state(context).full_stack.clear();
    }

    // create a file if the execution is normal.
    // end function is only called if the execution is normal,
    // so this file will only be created if everything goes fine.
    std::string database_filepath =
        tracer_serializer(context).get_database_filepath();
    size_t lastindex = database_filepath.find_last_of(".");
    std::string ok_filepath = database_filepath.substr(0, lastindex) + ".OK";
    std::ofstream ok_file(ok_filepath);
    ok_file << "NORMAL EXIT";
    ok_file.close();
}

// Triggered when entering function evaluation.
void function_entry(dyntrace_context_t *context, const SEXP call, const SEXP op,
                    const SEXP rho) {
    closure_info_t info = function_entry_get_info(context, call, op, rho);

    stack_event_t stack_elem;
    stack_elem.type = stack_type::CALL;
    stack_elem.call_id = info.call_id;
    stack_elem.enclosing_environment = info.call_ptr;
    tracer_state(context).full_stack.push_back(stack_elem);

    // Push function ID on function stack
//    call_stack_elem_t e;
//    e.call_id = info.call_id;
//    e.function_id = info.fn_id;
//    e.type = info.fn_type;
//    e.enclosing_environment = info.call_ptr;
//    tracer_state(context).fun_stack.push_back(e);
    //tracer_state(context).curr_env_stack.push(info.call_ptr);

    debug_serializer(context).serialize_function_entry(info);
    tracer_serializer(context).serialize_function_entry(context, info);

    auto &fresh_promises = tracer_state(context).fresh_promises;
    // Associate promises with call ID
    for (auto arg_ref : info.arguments.all()) {
        const arg_t &argument = arg_ref.get();
        auto &promise = get<2>(argument);
        auto it = fresh_promises.find(promise);
        // if promise environment is same as the caller's environment, then
        // serialize this promise as it is a default argument.

        debug_serializer(context).serialize_promise_argument_type(
            promise, get<3>(argument));
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
    closure_info_t info =
        function_exit_get_info(context, call, op, rho, retval);

    tracer_state(context).full_stack.pop_back();

    debug_serializer(context).serialize_function_exit(info);
    tracer_serializer(context).serialize_function_exit(info);

    // Current function ID is popped in function_exit_get_info
    // tracer_state(context).curr_env_stack.pop();
    // Moved to its own hook.
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

void builtin_exit(dyntrace_context_t *context, const SEXP call, const SEXP op,
                  const SEXP rho, const SEXP retval) {
    function_type fn_type;
    if (TYPEOF(op) == BUILTINSXP)
        fn_type = (PRIMINTERNAL(op) == 0) ? function_type::TRUE_BUILTIN
                                          : function_type::BUILTIN;
    else
        fn_type = function_type::TRUE_BUILTIN;
    print_exit_info(context, call, op, rho, fn_type, retval);
}

void specialsxp_entry(dyntrace_context_t *context, const SEXP call,
                      const SEXP op, const SEXP rho) {
    print_entry_info(context, call, op, rho, function_type::SPECIAL);
}

void specialsxp_exit(dyntrace_context_t *context, const SEXP call,
                     const SEXP op, const SEXP rho, const SEXP retval) {
    print_exit_info(context, call, op, rho, function_type::SPECIAL, retval);
}

void print_entry_info(dyntrace_context_t *context, const SEXP call,
                      const SEXP op, const SEXP rho, function_type fn_type) {
    builtin_info_t info =
        builtin_entry_get_info(context, call, op, rho, fn_type);

    stack_event_t stack_elem;
    stack_elem.type = stack_type::CALL;
    stack_elem.call_id = info.call_id;
    stack_elem.enclosing_environment = info.call_ptr;
    tracer_state(context).full_stack.push_back(stack_elem);

    //call_stack_elem_t e;
    //e.call_id = info.call_id;
    //e.function_id = info.fn_id;
    //e.type = info.fn_type;
    //e.enclosing_environment = info.call_ptr;
//    tracer_state(context).fun_stack.push_back(e);

    debug_serializer(context).serialize_builtin_entry(info);
    tracer_serializer(context).serialize_builtin_entry(context, info);

    // Moved to its own hook.
    // tracer_state(context).curr_env_stack.push(info.call_ptr | 1);
}

void print_exit_info(dyntrace_context_t *context, const SEXP call,
                     const SEXP op, const SEXP rho, function_type fn_type,
                     const SEXP retval) {
    builtin_info_t info =
        builtin_exit_get_info(context, call, op, rho, fn_type, retval);

    tracer_state(context).full_stack.pop_back(); // TODO add check

    debug_serializer(context).serialize_builtin_exit(info);
    tracer_serializer(context).serialize_builtin_exit(info);

    // Moved to its own hook.
    // tracer_state(context).curr_env_stack.pop();
}

void promise_created(dyntrace_context_t *context, const SEXP prom,
                     const SEXP rho) {
    prom_basic_info_t info = create_promise_get_info(context, prom, rho);
    debug_serializer(context).serialize_promise_created(info);
    tracer_serializer(context).serialize_promise_created(info);
    if (info.prom_id >= 0) { // maybe we don't need this check
        prom_gc_info_t prom_gc_info = {info.prom_id, 0, tracer_state(context).gc_trigger_counter};
        debug_serializer(context).serialize_promise_lifecycle(prom_gc_info);
        tracer_serializer(context).serialize_promise_lifecycle(prom_gc_info);
    }
    std::string cre_id = std::string("cre ") + std::to_string(info.prom_id);
    debug_serializer(context).serialize_interference_information(cre_id);
    tracer_serializer(context).serialize_interference_information(cre_id);
}

// Promise is being used inside a function body for the first time.
void promise_force_entry(dyntrace_context_t *context, const SEXP promise) {
    prom_info_t info = force_promise_entry_get_info(context, promise);

    stack_event_t stack_elem;
    stack_elem.type = stack_type::PROMISE;
    stack_elem.promise_id = info.prom_id;
    stack_elem.enclosing_environment = tracer_state(context).full_stack.back().enclosing_environment; // FIXME necessary?
    tracer_state(context).full_stack.push_back(stack_elem);

    std::string ent_id = std::string("ent ") + std::to_string(info.prom_id);
    debug_serializer(context).serialize_interference_information(ent_id);
    tracer_serializer(context).serialize_interference_information(ent_id);
    debug_serializer(context).serialize_force_promise_entry(info);
    tracer_serializer(context).serialize_force_promise_entry(
        context, info, tracer_state(context).clock_id);
    tracer_state(context).clock_id++;
    /* reset number of environment bindings. We want to know how many bindings
       happened while forcing this promise */
    if (info.prom_id >= 0) {
        prom_gc_info_t prom_gc_info = {info.prom_id, 1, tracer_state(context).gc_trigger_counter};
        debug_serializer(context).serialize_promise_lifecycle(prom_gc_info);
        tracer_serializer(context).serialize_promise_lifecycle(prom_gc_info);
    }
}

void promise_force_exit(dyntrace_context_t *context, const SEXP promise) {
    prom_info_t info = force_promise_exit_get_info(context, promise);

    tracer_state(context).full_stack.pop_back();

    std::string ext_id = std::string("ext ") + std::to_string(info.prom_id);
    debug_serializer(context).serialize_interference_information(ext_id);
    tracer_serializer(context).serialize_interference_information(ext_id);
    debug_serializer(context).serialize_force_promise_exit(info);
    tracer_serializer(context).serialize_force_promise_exit(
        info, tracer_state(context).clock_id);
    tracer_state(context).clock_id++;
}

void promise_value_lookup(dyntrace_context_t *context, const SEXP promise) {
    prom_info_t info = promise_lookup_get_info(context, promise);
    std::string val_id = std::string("val ") + std::to_string(info.prom_id);
    debug_serializer(context).serialize_interference_information(val_id);
    tracer_serializer(context).serialize_interference_information(val_id);
    if (info.prom_id >= 0) {
        debug_serializer(context).serialize_promise_lookup(info);
        tracer_serializer(context).serialize_promise_lookup(
            info, tracer_state(context).clock_id);
        tracer_state(context).clock_id++;
        prom_gc_info_t prom_gc_info {info.prom_id, 1, tracer_state(context).gc_trigger_counter};
        debug_serializer(context).serialize_promise_lifecycle(prom_gc_info);
        tracer_serializer(context).serialize_promise_lifecycle(prom_gc_info);
    }
}

void promise_expression_lookup(dyntrace_context_t *context, const SEXP prom) {
    prom_info_t info = promise_expression_lookup_get_info(context, prom);
    if (info.prom_id >= 0) {
        debug_serializer(context).serialize_promise_expression_lookup(info);
        tracer_serializer(context).serialize_promise_expression_lookup(
            info, tracer_state(context).clock_id);
        tracer_state(context).clock_id++;
        prom_gc_info_t prom_gc_info = {info.prom_id, 3, tracer_state(context).gc_trigger_counter};
        debug_serializer(context).serialize_promise_lifecycle(prom_gc_info);
        tracer_serializer(context).serialize_promise_lifecycle(prom_gc_info);
    }
}

void gc_promise_unmarked(dyntrace_context_t *context, const SEXP promise) {
    prom_addr_t addr = get_sexp_address(promise);
    prom_id_t id = get_promise_id(context, promise);
    auto &promise_origin = tracer_state(context).promise_origin;

    if (id >= 0) {
        prom_gc_info_t prom_gc_info = {id, 2, tracer_state(context).gc_trigger_counter};
        debug_serializer(context).serialize_promise_lifecycle(prom_gc_info);
        tracer_serializer(context).serialize_promise_lifecycle(prom_gc_info);
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
    debug_serializer(context).serialize_gc_exit(info);
    tracer_serializer(context).serialize_gc_exit(info);
}

void vector_alloc(dyntrace_context_t *context, int sexptype, long length,
                  long bytes, const char *srcref) {
    //type_gc_info_t info{tracer_state(context).get_gc_trigger_counter(),
    //                    sexptype, length, bytes};
    //debug_serializer(context).serialize_vector_alloc(info);
    //tracer_serializer(context).serialize_vector_alloc(info);
}

void new_environment(dyntrace_context_t *context, const SEXP rho) {
    //fn_id_t fn_id = (tracer_state(context).fun_stack.back().function_id);
    stack_event_t event = get_last_on_stack_by_type(tracer_state(context).full_stack, stack_type::CALL);
    fn_id_t fn_id = event.type == stack_type::NONE ? compute_hash("") : event.function_info.function_id;

    env_id_t env_id = tracer_state(context).environment_id_counter++;
    debug_serializer(context).serialize_new_environment(env_id, fn_id);
    tracer_serializer(context).serialize_new_environment(env_id, fn_id);
    tracer_state(context).environments[rho] =
        std::pair<env_id_t, unordered_map<string, var_id_t>>(env_id, {});
}

void begin_ctxt(dyntrace_context_t *context, const RCNTXT *cptr) {
//    context_t ctx;
//    ctx.environment = (env_addr_t) cptr->cloenv;
//    ctx.context = (rid_t) cptr;
//    tracer_state(context).curr_env_stack.push_back(ctx);

    stack_event_t event;
    event.context_id = (rid_t) cptr;
    event.type = stack_type::CONTEXT;
    tracer_state(context).full_stack.push_back(event);

    debug_serializer(context).serialize_begin_ctxt(cptr);
}

void jump_ctxt(dyntrace_context_t *context, const RCNTXT *cptr) {
    vector<call_id_t> unwound_calls;
    vector<prom_id_t> unwound_promises;
    unwind_info_t info;
    info.jump_target = ((env_addr_t) cptr->cloenv);
    info.jump_context = ((rid_t) cptr);

    tracer_state(context).adjust_stacks(info);

    debug_serializer(context).serialize_unwind(info);
    tracer_serializer(context).serialize_unwind(info);
}

void end_ctxt(dyntrace_context_t *context, const RCNTXT *cptr) {
//    if (((rid_t)cptr->cloenv) == tracer_state(context).curr_env_stack.back().environment)
//        tracer_state(context).curr_env_stack.pop_back();
//    else
//        dyntrace_log_warning("Context trying to remove environment %d from stack, but %d is on top of stack.",
//                            ((rid_t) cptr->cloenv), tracer_state(context).curr_env_stack.back());

    stack_event_t event = tracer_state(context).full_stack.back();
    if (event.type == stack_type::CONTEXT && ((rid_t) cptr) == event.context_id)
        tracer_state(context).full_stack.pop_back();
    else
        dyntrace_log_warning("Context trying to remove context %d from full stack, but %d is on top of stack.",
                             ((rid_t) cptr), event.context_id);

    debug_serializer(context).serialize_end_ctxt(cptr);
}

void environment_action(dyntrace_context_t *context, const SEXP symbol,
                        const SEXP rho, const std::string &action) {
    bool exists = true;
    prom_id_t promise_id = tracer_state(context).enclosing_promise_id();
    env_id_t environment_id = tracer_state(context).to_environment_id(rho);
    var_id_t variable_id =
        tracer_state(context).to_variable_id(symbol, rho, exists);
    // serialize variable iff it has not been seen before.
    // if it has been seen before, then it has already been serialized.
    if (!exists) {
        debug_serializer(context).serialize_variable(
            variable_id, CHAR(PRINTNAME(symbol)), environment_id);
        tracer_serializer(context).serialize_variable(
            variable_id, CHAR(PRINTNAME(symbol)), environment_id);
    }
    debug_serializer(context).serialize_variable_action(promise_id,
                                                        variable_id, action);
    tracer_serializer(context).serialize_variable_action(promise_id,
                                                         variable_id, action);
    std::string action_id = action + " " + std::to_string(variable_id);
    debug_serializer(context).serialize_interference_information(action_id);
    tracer_serializer(context).serialize_interference_information(action_id);
}

void environment_define_var(dyntrace_context_t *context, const SEXP symbol,
                            const SEXP value, const SEXP rho) {
    environment_action(context, symbol, rho, "def");
}

void environment_assign_var(dyntrace_context_t *context, const SEXP symbol,
                            const SEXP value, const SEXP rho) {
    environment_action(context, symbol, rho, "asn");
}

void environment_remove_var(dyntrace_context_t *context, const SEXP symbol,
                            const SEXP rho) {
    environment_action(context, symbol, rho, "rem");
}

void environment_lookup_var(dyntrace_context_t *context, const SEXP symbol,
                            const SEXP value, const SEXP rho) {
    environment_action(context, symbol, rho, "rea");
}
