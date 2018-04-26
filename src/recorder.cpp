#include "recorder.h"
#include "lookup.h"

#ifdef RDT_TIMER
#include "timer.h"
using namespace timing;
#endif

recursion_type is_recursive(dyntrace_context_t *context, fn_id_t function) {
    for (vector<stack_event_t>::reverse_iterator i =
             tracer_state(context).full_stack.rbegin();
         i != tracer_state(context).full_stack.rend(); ++i) {

        if (i->type != stack_type::CALL) {
            continue;
        }

        if (i->function_info.function_id == function) {
            return recursion_type::RECURSIVE;
        }

        if (i->function_info.type == function_type::BUILTIN ||
            i->function_info.type == function_type::CLOSURE) {
            return recursion_type::NOT_RECURSIVE;
        }

        // inside a different function, but one that doesn't matter, recursion
        // still possible
    }
    return recursion_type::UNKNOWN;
}

tuple<lifestyle_type, int, int>
judge_promise_lifestyle(dyntrace_context_t *context, call_id_t from_call_id) {
    int effective_distance = 0;
    int actual_distance = 0;
    for (vector<stack_event_t>::reverse_iterator i =
             tracer_state(context).full_stack.rbegin();
         i != tracer_state(context).full_stack.rend(); ++i) {

        if (i->type != stack_type::CALL)
            continue;

        if (i->call_id == from_call_id) {
            if (effective_distance == 0) {
                if (actual_distance == 0) {
                    return tuple<lifestyle_type, int, int>(
                        lifestyle_type::IMMEDIATE_LOCAL, effective_distance,
                        actual_distance);
                } else {
                    return tuple<lifestyle_type, int, int>(
                        lifestyle_type::LOCAL, effective_distance,
                        actual_distance);
                }
            } else {
                if (effective_distance == 1) {
                    return tuple<lifestyle_type, int, int>(
                        lifestyle_type::IMMEDIATE_BRANCH_LOCAL,
                        effective_distance, actual_distance);
                } else {
                    return tuple<lifestyle_type, int, int>(
                        lifestyle_type::BRANCH_LOCAL, effective_distance,
                        actual_distance);
                }
            }
        }

        if (i->call_id == 0) {
            return tuple<lifestyle_type, int, int>(
                lifestyle_type::ESCAPED, -1,
                -1); // reached root, parent must be in a
                     // different branch--promise escaped
        }

        actual_distance++;
        if (i->function_info.type == function_type::BUILTIN ||
            i->function_info.type == function_type::CLOSURE) {
            effective_distance++;
        }
    }
}

void set_distances_and_lifestyle(dyntrace_context_t *context,
                                 prom_info_t &info) {
    if (info.in_call_id == info.from_call_id) {
        info.lifestyle = lifestyle_type::LOCAL;
        info.effective_distance_from_origin = 0;
        info.actual_distance_from_origin = 0;
    } else {
        auto lifestyle_info =
            judge_promise_lifestyle(context, info.from_call_id);
        info.lifestyle = get<0>(lifestyle_info);
        info.effective_distance_from_origin = get<1>(lifestyle_info);
        info.actual_distance_from_origin = get<2>(lifestyle_info);
    }
}

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
        argument.argument_type = sexp_type::DOT;
    } else {
        argument.argument_type = static_cast<sexp_type>(argument_type);
    }

    argument.id = get_argument_id(context, call_id, argument.name);
    argument.expression_type = static_cast<sexp_type>(expression_type);
    argument.formal_parameter_position = position;

    info.arguments.push_back(argument);
}

void update_closure_arguments(closure_info_t &info, dyntrace_context_t *context,
                              const call_id_t call_id, const SEXP op,
                              const SEXP environment) {

    int formal_position = 0;
    SEXP formals = FORMALS(op);
    for (; formals != R_NilValue; formals = CDR(formals), formal_position++) {

        // Retrieve the argument name.
        SEXP argument_expression = TAG(formals);

        SEXP expression;
        // We want the promise associated with the symbol.
        // Generally, the argument_expression should be the promise.
        // But if JIT is enabled, its possible for the argument_expression
        // to be unpromised. In this case, we dereference the argument.
        if (TYPEOF(argument_expression) == SYMSXP) {
            lookup_result r =
                find_binding_in_environment(argument_expression, environment);
            if (r.status == lookup_status::SUCCESS) {
                expression = r.value;
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
            expression = argument_expression;
        }

        // Encountered a triple-dot argument, break it up further.
        if (TYPEOF(expression) == DOTSXP) {
            for (SEXP dots = expression; dots != R_NilValue; dots = CDR(dots)) {
                update_closure_argument(info, context, call_id, TAG(dots),
                                        CAR(dots), environment, true,
                                        formal_position);
            }
            return;
        }

        // The general case: single argument.
        update_closure_argument(info, context, call_id, argument_expression,
                                expression, environment, false,
                                formal_position);
    }
}

closure_info_t function_entry_get_info(dyntrace_context_t *context,
                                       const SEXP call, const SEXP op,
                                       const SEXP rho) {
#ifdef RDT_TIMER
    Timer::getInstance(timer::RECORDER).reset();
#endif

    closure_info_t info;

    const char *name = get_name(call);
    const char *ns = get_ns_name(op);

    info.fn_compiled = is_byte_compiled(op);
    info.fn_type = function_type::CLOSURE;
#ifdef RDT_TIMER
    Timer::getInstance(timer::RECORDER)
        .endSegment(segment::FUNCTION_ENTRY_RECORDER_OTHER);
#endif
    info.fn_definition = get_function_definition(context, op);
#ifdef RDT_TIMER
    Timer::getInstance(timer::RECORDER)
        .endSegment(segment::FUNCTION_ENTRY_RECORDER_DEFINITION);
#endif
    info.fn_id = get_function_id(context, info.fn_definition);
#ifdef RDT_TIMER
    Timer::getInstance(timer::RECORDER)
        .endSegment(segment::FUNCTION_ENTRY_RECORDER_FUNCTION_ID);
#endif
    info.fn_addr = get_function_addr(op);
    info.call_ptr = get_sexp_address(rho);
#ifdef RDT_TIMER
    Timer::getInstance(timer::RECORDER)
        .endSegment(segment::FUNCTION_ENTRY_RECORDER_OTHER);
#endif
    info.call_id = make_funcall_id(context, op);
#ifdef RDT_TIMER
    Timer::getInstance(timer::RECORDER)
        .endSegment(segment::FUNCTION_ENTRY_RECORDER_CALL_ID);
#endif

    stack_event_t event = get_last_on_stack_by_type(
        tracer_state(context).full_stack, stack_type::CALL);
    info.parent_call_id = event.type == stack_type::NONE ? 0 : event.call_id;
#ifdef RDT_TIMER
    Timer::getInstance(timer::RECORDER)
        .endSegment(segment::FUNCTION_ENTRY_RECORDER_PARENT_ID);
#endif

    info.definition_location = get_definition_location_cpp(op);
    info.callsite_location = get_callsite_cpp(1);
#ifdef RDT_TIMER
    Timer::getInstance(timer::RECORDER)
        .endSegment(segment::FUNCTION_ENTRY_RECORDER_LOCATION);
#endif

    void (*probe)(dyntrace_context_t *, SEXP, int);
    probe = dyntrace_active_dyntracer->probe_promise_expression_lookup;
    dyntrace_active_dyntracer->probe_promise_expression_lookup = NULL;
    info.call_expression = get_expression(call);
#ifdef RDT_TIMER
    Timer::getInstance(timer::RECORDER)
        .endSegment(segment::FUNCTION_ENTRY_RECORDER_EXPRESSION);
#endif

    dyntrace_active_dyntracer->probe_promise_expression_lookup = probe;
    if (ns) {
        info.name = string(ns) + "::" + CHKSTR(name);
    } else {
        if (name != NULL)
            info.name = name;
    }
#ifdef RDT_TIMER
    Timer::getInstance(timer::RECORDER)
        .endSegment(segment::FUNCTION_ENTRY_RECORDER_NAME);
#endif

    update_closure_arguments(info, context, info.call_id, op, rho);
#ifdef RDT_TIMER
    Timer::getInstance(timer::RECORDER)
        .endSegment(segment::FUNCTION_ENTRY_RECORDER_ARGUMENTS);
#endif
    info.recursion = is_recursive(context, info.fn_id);
#ifdef RDT_TIMER
    Timer::getInstance(timer::RECORDER)
        .endSegment(segment::FUNCTION_ENTRY_RECORDER_RECURSIVE);
#endif

    get_stack_parent(info, tracer_state(context).full_stack);
    info.in_prom_id = get_parent_promise(context);
#ifdef RDT_TIMER
    Timer::getInstance(timer::RECORDER)
        .endSegment(segment::FUNCTION_ENTRY_RECORDER_PARENT_PROMISE);
#endif

    return info;
}

closure_info_t function_exit_get_info(dyntrace_context_t *context,
                                      const SEXP call, const SEXP op,
                                      const SEXP rho, const SEXP retval) {
#ifdef RDT_TIMER
    Timer::getInstance(timer::RECORDER).reset();
#endif

    closure_info_t info;

    const char *name = get_name(call);
    const char *ns = get_ns_name(op);

    info.fn_compiled = is_byte_compiled(op);
#ifdef RDT_TIMER
    Timer::getInstance(timer::RECORDER)
        .endSegment(segment::FUNCTION_EXIT_RECORDER_OTHER);
#endif
    info.fn_definition = get_function_definition(context, op);
#ifdef RDT_TIMER
    Timer::getInstance(timer::RECORDER)
        .endSegment(segment::FUNCTION_EXIT_RECORDER_DEFINITION);
#endif
    info.fn_id = get_function_id(context, info.fn_definition);
#ifdef RDT_TIMER
    Timer::getInstance(timer::RECORDER)
        .endSegment(segment::FUNCTION_EXIT_RECORDER_FUNCTION_ID);
#endif
    info.fn_addr = get_function_addr(op);

    stack_event_t call_event = get_last_on_stack_by_type(
        tracer_state(context).full_stack, stack_type::CALL);
    info.call_id = call_event.type == stack_type::NONE ? 0 : call_event.call_id;
#ifdef RDT_TIMER
    Timer::getInstance(timer::RECORDER)
        .endSegment(segment::FUNCTION_EXIT_RECORDER_CALL_ID);
#endif

    info.fn_type = function_type::CLOSURE;
#ifdef RDT_TIMER
    Timer::getInstance(timer::RECORDER)
        .endSegment(segment::FUNCTION_EXIT_RECORDER_OTHER);
#endif

    info.definition_location = get_definition_location_cpp(op);
    info.callsite_location = get_callsite_cpp(0);
#ifdef RDT_TIMER
    Timer::getInstance(timer::RECORDER)
        .endSegment(segment::FUNCTION_EXIT_RECORDER_LOCATION);
#endif

    if (ns) {
        info.name = string(ns) + "::" + CHKSTR(name);
    } else {
        if (name != NULL)
            info.name = name;
    }
#ifdef RDT_TIMER
    Timer::getInstance(timer::RECORDER)
        .endSegment(segment::FUNCTION_EXIT_RECORDER_NAME);
#endif

    update_closure_arguments(info, context, info.call_id, op, rho);
#ifdef RDT_TIMER
    Timer::getInstance(timer::RECORDER)
        .endSegment(segment::FUNCTION_EXIT_RECORDER_ARGUMENTS);
#endif

    stack_event_t parent_call = get_from_back_of_stack_by_type(
        tracer_state(context).full_stack, stack_type::CALL, 1);
    info.parent_call_id =
        parent_call.type == stack_type::NONE ? 0 : parent_call.call_id;
#ifdef RDT_TIMER
    Timer::getInstance(timer::RECORDER)
        .endSegment(segment::FUNCTION_EXIT_RECORDER_PARENT_ID);
#endif
    info.recursion = is_recursive(context, info.fn_id);
#ifdef RDT_TIMER
    Timer::getInstance(timer::RECORDER)
        .endSegment(segment::FUNCTION_EXIT_RECORDER_RECURSIVE);
#endif

    get_stack_parent2(info, tracer_state(context).full_stack);
    info.in_prom_id = get_parent_promise(context);
#ifdef RDT_TIMER
    Timer::getInstance(timer::RECORDER)
        .endSegment(segment::FUNCTION_EXIT_RECORDER_PARENT_PROMISE);
#endif
    info.return_value_type = static_cast<sexp_type>(TYPEOF(retval));
#ifdef RDT_TIMER
    Timer::getInstance(timer::RECORDER)
        .endSegment(segment::FUNCTION_EXIT_RECORDER_OTHER);
#endif
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
    info.recursion = is_recursive(context, info.fn_id);

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
    info.recursion = is_recursive(context, info.fn_id);
    info.definition_location = get_definition_location_cpp(op);
    info.callsite_location = get_callsite_cpp(0);

    stack_event_t parent_call = get_from_back_of_stack_by_type(
        tracer_state(context).full_stack, stack_type::CALL, 1);
    info.parent_call_id =
        parent_call.type == stack_type::NONE ? 0 : parent_call.call_id;

    get_stack_parent2(info, tracer_state(context).full_stack);
    info.in_prom_id = get_parent_promise(context);
    info.return_value_type = static_cast<sexp_type>(TYPEOF(retval));

    return info;
}

prom_basic_info_t create_promise_get_info(dyntrace_context_t *context,
                                          const SEXP promise, const SEXP rho) {
    prom_basic_info_t info;

    info.prom_id = make_promise_id(context, promise);
    tracer_state(context).fresh_promises.insert(info.prom_id);

    info.prom_type = static_cast<sexp_type>(TYPEOF(PRCODE(promise)));
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

    set_distances_and_lifestyle(context, info);

    info.prom_type = static_cast<sexp_type>(TYPEOF(PRCODE(promise)));
    get_full_type(promise, info.full_type);
    info.return_type = sexp_type::OMEGA;
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

    set_distances_and_lifestyle(context, info);

    info.prom_type = static_cast<sexp_type>(TYPEOF(PRCODE(promise)));
    get_full_type(promise, info.full_type);
    info.return_type = static_cast<sexp_type>(TYPEOF(PRVALUE(promise)));

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

    set_distances_and_lifestyle(context, info);

    info.prom_type = static_cast<sexp_type>(TYPEOF(PRCODE(promise)));
    info.full_type.push_back(sexp_type::OMEGA);
    info.return_type = static_cast<sexp_type>(TYPEOF(PRVALUE(promise)));

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

    set_distances_and_lifestyle(context, info);

    info.prom_type = static_cast<sexp_type>(TYPEOF(PRCODE(prom)));
    info.full_type.push_back(sexp_type::OMEGA);
    info.return_type = static_cast<sexp_type>(TYPEOF(PRCODE(prom)));

    return info;
}
