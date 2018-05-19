#include "recorder.h"
#include "Timer.h"
#include "lookup.h"

void update_closure_argument(closure_info_t &info, dyntrace_context_t *context,
                             call_id_t call_id, const SEXP argument_expression,
                             const SEXP expression, const SEXP environment,
                             bool dot_argument, int position) {

    arg_t argument;

    SEXPTYPE expression_type = TYPEOF(expression);
    SEXPTYPE argument_type = TYPEOF(argument_expression);

    if (argument_expression != R_NilValue) {
        argument.name = string(get_name(argument_expression));
    }

    if (expression_type == PROMSXP) {
        argument.promise_id = get_promise_id(context, expression);
        argument.default_argument = (PRENV(expression) == environment);
    } else {
        argument.promise_id = 0;
        argument.default_argument = true;
    }

    if (dot_argument) {
        argument.argument_type = (sexptype_t)DOTSXP;
    } else {
        argument.argument_type = static_cast<sexptype_t>(argument_type);
    }

    argument.id = get_argument_id(context, call_id, argument.name);
    argument.expression_type = static_cast<sexptype_t>(expression_type);
    argument.formal_parameter_position = position;

    info.arguments.push_back(argument);
}

void update_closure_arguments(closure_info_t &info, dyntrace_context_t *context,
                              const call_id_t call_id, const SEXP op,
                              const SEXP environment) {

    int formal_parameter_position = 0;
    SEXP arg_name = R_NilValue;
    SEXP arg_value = R_NilValue;

    for (SEXP formals = FORMALS(op); formals != R_NilValue;
         formals = CDR(formals), formal_parameter_position++) {

        // Retrieve the argument name.
        arg_name = TAG(formals);

        // We want the promise associated with the symbol.
        // Generally, the argument_expression should be the promise.
        // But if JIT is enabled, its possible for the argument_expression
        // to be unpromised. In this case, we dereference the argument.
        if (TYPEOF(arg_name) == SYMSXP) {
            lookup_result r =
                find_binding_in_environment(arg_name, environment);
            if (r.status == lookup_status::SUCCESS) {
                arg_value = r.value;
            } else {
                // So... since this is a function, then I assume we shouldn't
                // get any arguments that are active bindings or anything like
                // that. If we do, then we should fail here and re-think our
                // life choices.
                string msg = lookup_status_to_string(r.status);
                dyntrace_log_error("%s", msg.c_str());
            }
        }
        // If the argument already has an associated promise, then use that.
        // In case we see something else like NILSXP, the various helper
        // functions
        // below should be geared to deal with it.
        else {
            arg_value = arg_name;
        }

        // Encountered a triple-dot argument, break it up further.
        if (TYPEOF(arg_value) == DOTSXP) {
            for (SEXP dot_args = arg_value; dot_args != R_NilValue;
                 dot_args = CDR(dot_args)) {
                update_closure_argument(info, context, call_id, TAG(dot_args),
                                        CAR(dot_args), environment, true,
                                        formal_parameter_position);
            }
        }

        // The general case: single argument.
        update_closure_argument(info, context, call_id, arg_name, arg_value,
                                environment, false, formal_parameter_position);
    }
}

closure_info_t function_entry_get_info(dyntrace_context_t *context,
                                       const SEXP call, const SEXP op,
                                       const SEXP rho) {
    RECORDER_TIMER_RESET();
    closure_info_t info;

    const char *name = get_name(call);
    const char *ns = get_ns_name(op);

    info.fn_compiled = is_byte_compiled(op);
    info.fn_type = function_type::CLOSURE;
    RECORDER_TIMER_END_SEGMENT(FUNCTION_ENTRY_RECORDER_OTHER);

    info.fn_definition = get_function_definition(context, op);
    RECORDER_TIMER_END_SEGMENT(FUNCTION_ENTRY_RECORDER_DEFINITION);

    info.fn_id = get_function_id(context, info.fn_definition);
    RECORDER_TIMER_END_SEGMENT(FUNCTION_ENTRY_RECORDER_FUNCTION_ID);

    info.fn_addr = get_function_addr(op);
    info.call_ptr = get_sexp_address(rho);
    RECORDER_TIMER_END_SEGMENT(FUNCTION_ENTRY_RECORDER_OTHER);

    info.call_id = make_funcall_id(context, op);
    RECORDER_TIMER_END_SEGMENT(FUNCTION_ENTRY_RECORDER_CALL_ID);

    stack_event_t event = get_last_on_stack_by_type(
        tracer_state(context).full_stack, stack_type::CALL);
    info.parent_call_id = event.type == stack_type::NONE ? 0 : event.call_id;
    RECORDER_TIMER_END_SEGMENT(FUNCTION_ENTRY_RECORDER_PARENT_ID);

    info.definition_location = get_definition_location_cpp(op);
    info.callsite_location = get_callsite_cpp(1);
    RECORDER_TIMER_END_SEGMENT(FUNCTION_ENTRY_RECORDER_LOCATION);

    void (*probe)(dyntrace_context_t *, SEXP, int);
    probe = dyntrace_active_dyntracer->probe_promise_expression_lookup;
    dyntrace_active_dyntracer->probe_promise_expression_lookup = NULL;
    info.call_expression = get_expression(call);
    RECORDER_TIMER_END_SEGMENT(FUNCTION_ENTRY_RECORDER_EXPRESSION);

    dyntrace_active_dyntracer->probe_promise_expression_lookup = probe;
    if (ns) {
        info.name = string(ns) + "::" + CHKSTR(name);
    } else {
        if (name != NULL)
            info.name = name;
    }
    RECORDER_TIMER_END_SEGMENT(FUNCTION_ENTRY_RECORDER_NAME);

    update_closure_arguments(info, context, info.call_id, op, rho);
    RECORDER_TIMER_END_SEGMENT(FUNCTION_ENTRY_RECORDER_ARGUMENTS);

    get_stack_parent(info, tracer_state(context).full_stack);
    info.in_prom_id = get_parent_promise(context);
    RECORDER_TIMER_END_SEGMENT(FUNCTION_ENTRY_RECORDER_PARENT_PROMISE);

    return info;
}

closure_info_t function_exit_get_info(dyntrace_context_t *context,
                                      const SEXP call, const SEXP op,
                                      const SEXP rho, const SEXP retval) {
    RECORDER_TIMER_RESET();
    closure_info_t info;

    const char *name = get_name(call);
    const char *ns = get_ns_name(op);

    info.fn_compiled = is_byte_compiled(op);
    RECORDER_TIMER_END_SEGMENT(FUNCTION_EXIT_RECORDER_OTHER);

    info.fn_definition = get_function_definition(context, op);
    RECORDER_TIMER_END_SEGMENT(FUNCTION_EXIT_RECORDER_DEFINITION);

    info.fn_id = get_function_id(context, info.fn_definition);
    RECORDER_TIMER_END_SEGMENT(FUNCTION_EXIT_RECORDER_FUNCTION_ID);

    info.fn_addr = get_function_addr(op);

    stack_event_t call_event = get_last_on_stack_by_type(
        tracer_state(context).full_stack, stack_type::CALL);
    info.call_id = call_event.type == stack_type::NONE ? 0 : call_event.call_id;
    RECORDER_TIMER_END_SEGMENT(FUNCTION_EXIT_RECORDER_CALL_ID);

    info.fn_type = function_type::CLOSURE;
    RECORDER_TIMER_END_SEGMENT(FUNCTION_EXIT_RECORDER_OTHER);

    info.definition_location = get_definition_location_cpp(op);
    info.callsite_location = get_callsite_cpp(0);
    RECORDER_TIMER_END_SEGMENT(FUNCTION_EXIT_RECORDER_LOCATION);

    if (ns) {
        info.name = string(ns) + "::" + CHKSTR(name);
    } else {
        if (name != NULL)
            info.name = name;
    }
    RECORDER_TIMER_END_SEGMENT(FUNCTION_EXIT_RECORDER_NAME);

    update_closure_arguments(info, context, info.call_id, op, rho);
    RECORDER_TIMER_END_SEGMENT(FUNCTION_EXIT_RECORDER_ARGUMENTS);

    info.fn_definition = get_expression(op);

    stack_event_t parent_call = get_from_back_of_stack_by_type(
        tracer_state(context).full_stack, stack_type::CALL, 1);
    info.parent_call_id =
        parent_call.type == stack_type::NONE ? 0 : parent_call.call_id;
    RECORDER_TIMER_END_SEGMENT(FUNCTION_EXIT_RECORDER_PARENT_ID);

    get_stack_parent2(info, tracer_state(context).full_stack);
    info.in_prom_id = get_parent_promise(context);
    RECORDER_TIMER_END_SEGMENT(FUNCTION_EXIT_RECORDER_PARENT_PROMISE);

    info.return_value_type = static_cast<sexptype_t>(TYPEOF(retval));
    RECORDER_TIMER_END_SEGMENT(FUNCTION_EXIT_RECORDER_OTHER);

    return info;
}

builtin_info_t builtin_entry_get_info(dyntrace_context_t *context,
                                      const SEXP call, const SEXP op,
                                      const SEXP rho, function_type fn_type) {
    builtin_info_t info;

    const char *name = get_name(call);
    if (name != NULL)
        info.name = name;
    info.fn_definition = get_function_definition(context, op);
    info.fn_id = get_function_id(context, info.fn_definition, true);
    info.fn_addr = get_function_addr(op);
    info.name = info.name;
    info.fn_type = fn_type;
    info.fn_compiled = is_byte_compiled(op);
    stack_event_t elem = get_last_on_stack_by_type(
        tracer_state(context).full_stack, stack_type::CALL);
    info.parent_call_id = elem.type == stack_type::NONE ? 0 : elem.call_id;
    info.definition_location = get_definition_location_cpp(op);
    info.callsite_location = get_callsite_cpp(0);
    info.call_ptr = get_sexp_address(rho);
    info.call_id = make_funcall_id(context, op);

    get_stack_parent(info, tracer_state(context).full_stack);
    info.in_prom_id = get_parent_promise(context);

    return info;
}

builtin_info_t builtin_exit_get_info(dyntrace_context_t *context,
                                     const SEXP call, const SEXP op,
                                     const SEXP rho, function_type fn_type,
                                     const SEXP retval) {
    builtin_info_t info;

    const char *name = get_name(call);
    if (name != NULL)
        info.name = name;
    info.fn_definition = get_function_definition(context, op);
    info.fn_id = get_function_id(context, info.fn_definition, true);
    info.fn_addr = get_function_addr(op);
    stack_event_t elem = get_last_on_stack_by_type(
        tracer_state(context).full_stack, stack_type::CALL);

    info.call_id = elem.type == stack_type::NONE ? 0 : elem.call_id;
    if (name != NULL)
        info.name = name;
    info.fn_type = fn_type;
    info.fn_compiled = is_byte_compiled(op);
    info.definition_location = get_definition_location_cpp(op);
    info.callsite_location = get_callsite_cpp(0);

    stack_event_t parent_call = get_from_back_of_stack_by_type(
        tracer_state(context).full_stack, stack_type::CALL, 1);
    info.parent_call_id =
        parent_call.type == stack_type::NONE ? 0 : parent_call.call_id;

    get_stack_parent2(info, tracer_state(context).full_stack);
    info.in_prom_id = get_parent_promise(context);
    info.return_value_type = static_cast<sexptype_t>(TYPEOF(retval));

    return info;
}

prom_basic_info_t create_promise_get_info(dyntrace_context_t *context,
                                          const SEXP promise, const SEXP rho) {
    prom_basic_info_t info;

    info.prom_id = make_promise_id(context, promise);
    tracer_state(context).fresh_promises.insert(info.prom_id);

    info.prom_type = static_cast<sexptype_t>(TYPEOF(PRCODE(promise)));
    get_full_type(promise, info.full_type);

    get_stack_parent(info, tracer_state(context).full_stack);
    info.in_prom_id = get_parent_promise(context);
    info.depth = get_no_of_ancestor_promises_on_stack(context);
    void (*probe)(dyntrace_context_t *, SEXP, int);
    probe = dyntrace_active_dyntracer->probe_promise_expression_lookup;
    dyntrace_active_dyntracer->probe_promise_expression_lookup = NULL;
    info.expression = get_expression(PRCODE(promise));
    dyntrace_active_dyntracer->probe_promise_expression_lookup = probe;
    info.expression_id = compute_hash(info.expression.c_str());
    return info;
}

prom_info_t force_promise_entry_get_info(dyntrace_context_t *context,
                                         const SEXP promise) {
    prom_info_t info;
    info.prom_id = get_promise_id(context, promise);

    stack_event_t elem = get_last_on_stack_by_type(
        tracer_state(context).full_stack, stack_type::CALL);
    info.in_call_id = elem.type == stack_type::NONE ? 0 : elem.call_id;
    info.from_call_id = tracer_state(context).promise_origin[info.prom_id];

    info.prom_type = static_cast<sexptype_t>(TYPEOF(PRCODE(promise)));
    get_full_type(promise, info.full_type);
    info.return_type = (sexptype_t)OMEGASXP;
    get_stack_parent(info, tracer_state(context).full_stack);
    info.in_prom_id = get_parent_promise(context);
    info.depth = get_no_of_ancestor_promises_on_stack(context);
    void (*probe)(dyntrace_context_t *, SEXP, int);
    probe = dyntrace_active_dyntracer->probe_promise_expression_lookup;
    dyntrace_active_dyntracer->probe_promise_expression_lookup = NULL;
    info.expression = get_expression(PRCODE(promise));
    dyntrace_active_dyntracer->probe_promise_expression_lookup = probe;
    info.expression_id = compute_hash(info.expression.c_str());
    return info;
}

prom_info_t force_promise_exit_get_info(dyntrace_context_t *context,
                                        const SEXP promise) {
    prom_info_t info;
    info.prom_id = get_promise_id(context, promise);

    stack_event_t elem = get_last_on_stack_by_type(
        tracer_state(context).full_stack, stack_type::CALL);
    info.in_call_id = elem.type == stack_type::NONE ? 0 : elem.call_id;
    info.from_call_id = tracer_state(context).promise_origin[info.prom_id];

    info.prom_type = static_cast<sexptype_t>(TYPEOF(PRCODE(promise)));
    get_full_type(promise, info.full_type);
    info.return_type = static_cast<sexptype_t>(TYPEOF(PRVALUE(promise)));

    get_stack_parent2(info, tracer_state(context).full_stack);
    info.in_prom_id = get_parent_promise(context);
    info.depth = get_no_of_ancestor_promises_on_stack(context);

    return info;
}

prom_info_t promise_lookup_get_info(dyntrace_context_t *context,
                                    const SEXP promise) {
    prom_info_t info;
    info.prom_id = get_promise_id(context, promise);

    stack_event_t elem = get_last_on_stack_by_type(
        tracer_state(context).full_stack, stack_type::CALL);
    info.in_call_id = elem.type == stack_type::NONE ? 0 : elem.call_id;
    info.from_call_id = tracer_state(context).promise_origin[info.prom_id];

    info.prom_type = static_cast<sexptype_t>(TYPEOF(PRCODE(promise)));
    info.full_type.push_back((sexptype_t)OMEGASXP);
    info.return_type = static_cast<sexptype_t>(TYPEOF(PRVALUE(promise)));

    get_stack_parent(info, tracer_state(context).full_stack);
    info.in_prom_id = get_parent_promise(context);
    info.depth = get_no_of_ancestor_promises_on_stack(context);

    return info;
}

prom_info_t promise_expression_lookup_get_info(dyntrace_context_t *context,
                                               const SEXP prom) {
    prom_info_t info;

    info.prom_id = get_promise_id(context, prom);

    stack_event_t elem = get_last_on_stack_by_type(
        tracer_state(context).full_stack, stack_type::CALL);
    info.in_call_id = elem.type == stack_type::NONE ? 0 : elem.call_id;
    info.from_call_id = tracer_state(context).promise_origin[info.prom_id];

    info.prom_type = static_cast<sexptype_t>(TYPEOF(PRCODE(prom)));
    info.full_type.push_back((sexptype_t)OMEGASXP);
    info.return_type = static_cast<sexptype_t>(TYPEOF(PRCODE(prom)));

    return info;
}
