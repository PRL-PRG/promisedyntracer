#include "recorder.h"

recursion_type is_recursive(dyntrace_context_t *context, fn_id_t function) {
    for (vector<call_stack_elem_t>::reverse_iterator i =
             tracer_state(context).fun_stack.rbegin();
         i != tracer_state(context).fun_stack.rend(); ++i) {
        call_id_t cursor_call = get<0>(*i);
        fn_id_t cursor_function = (get<1>(*i));
        function_type cursor_type = get<2>(*i);

        if (cursor_call == 0) {
            // end of stack
            return recursion_type::UNKNOWN;
        }

        if (cursor_function == function) {
            return recursion_type::RECURSIVE;
        }

        if (cursor_type == function_type::BUILTIN ||
            cursor_type == function_type::CLOSURE) {
            return recursion_type::NOT_RECURSIVE;
        }

        // inside a different function, but one that doesn't matter, recursion
        // still
        // possible
    }
}

tuple<lifestyle_type, int, int>
judge_promise_lifestyle(dyntrace_context_t *context, call_id_t from_call_id) {
    int effective_distance = 0;
    int actual_distance = 0;
    for (vector<call_stack_elem_t>::reverse_iterator i =
             tracer_state(context).fun_stack.rbegin();
         i != tracer_state(context).fun_stack.rend(); ++i) {
        call_id_t cursor = get<0>(*i);
        function_type type = get<2>(*i);

        if (cursor == from_call_id) {
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

        if (cursor == 0) {
            return tuple<lifestyle_type, int, int>(
                lifestyle_type::ESCAPED, -1,
                -1); // reached root, parent must be in a
                     // different branch--promise escaped
        }

        actual_distance++;
        if (type == function_type::BUILTIN || type == function_type::CLOSURE) {
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

closure_info_t function_entry_get_info(dyntrace_context_t *context,
                                       const SEXP call, const SEXP op,
                                       const SEXP rho) {
    closure_info_t info;

    const char *name = get_name(call);
    const char *ns = get_ns_name(op);

    info.fn_compiled = is_byte_compiled(op);
    info.fn_type = function_type::CLOSURE;
    info.fn_id = get_function_id(context, op);
    info.fn_addr = get_function_addr(op);
    info.call_ptr = get_sexp_address(rho);
    info.call_id = make_funcall_id(context, op);
    // info.call_id = make_funcall_id(rho);

    call_stack_elem_t elem = (tracer_state(context).fun_stack.back());
    info.parent_call_id = get<0>(elem);
    info.definition_location = get_definition_location_cpp(op);
    info.callsite_location = get_callsite_cpp(1);

    if (ns) {
        info.name = string(ns) + "::" + CHKSTR(name);
    } else {
        if (name != NULL)
            info.name = name;
    }

    info.arguments = get_arguments(context, info.call_id, op, rho);
    info.fn_definition = get_expression(op);

    info.recursion = is_recursive(context, info.fn_id);

    get_stack_parent(info, tracer_state(context).full_stack);
    info.in_prom_id = get_parent_promise(context);

    stack_event_t stack_elem;
    stack_elem.type = stack_type::CALL;
    stack_elem.call_id = info.call_id;
    tracer_state(context).full_stack.push_back(stack_elem);

    return info;
}

closure_info_t function_exit_get_info(dyntrace_context_t *context,
                                      const SEXP call, const SEXP op,
                                      const SEXP rho, const SEXP retval) {
    closure_info_t info;

    const char *name = get_name(call);
    const char *ns = get_ns_name(op);

    info.fn_compiled = is_byte_compiled(op);
    info.fn_id = get_function_id(context, op);
    info.fn_addr = get_function_addr(op);
    call_stack_elem_t elem = (tracer_state(context).fun_stack.back());
    info.call_id = get<0>(elem);
    info.fn_type = function_type::CLOSURE;

    info.definition_location = get_definition_location_cpp(op);
    info.callsite_location = get_callsite_cpp(0);

    if (ns) {
        info.name = string(ns) + "::" + CHKSTR(name);
    } else {
        if (name != NULL)
            info.name = name;
    }

    info.arguments = get_arguments(context, info.call_id, op, rho);
    info.fn_definition = get_expression(op);

    tracer_state(context).fun_stack.pop_back();
    call_stack_elem_t elem_parent = (tracer_state(context).fun_stack.back());
    info.parent_call_id = get<0>(elem_parent);

    info.recursion = is_recursive(context, info.fn_id);

    tracer_state(context).full_stack.pop_back();
    get_stack_parent(info, tracer_state(context).full_stack);
    info.in_prom_id = get_parent_promise(context);
    info.return_value_type = static_cast<sexp_type>(TYPEOF(retval));
    return info;
}

builtin_info_t builtin_entry_get_info(dyntrace_context_t *context,
                                      const SEXP call, const SEXP op,
                                      const SEXP rho, function_type fn_type) {
    builtin_info_t info;

    const char *name = get_name(call);
    if (name != NULL)
        info.name = name;
    info.fn_id = get_function_id(context, op, true);
    info.fn_addr = get_function_addr(op);
    info.name = info.name;
    info.fn_type = fn_type;
    info.fn_compiled = is_byte_compiled(op);
    info.fn_definition = get_expression(op);

    // R_FunTab[PRIMOFFSET(op)].eval % 100 )/10 ==

    call_stack_elem_t elem = (tracer_state(context).fun_stack.back());
    info.parent_call_id = get<0>(elem);

    info.definition_location = get_definition_location_cpp(op);
    info.callsite_location = get_callsite_cpp(0);

    info.call_ptr = get_sexp_address(rho);
    info.call_id = make_funcall_id(context, op);

    // XXX This is a remnant of an RDT_CALL_ID ifdef
    // Builtins have no environment of their own
    // we take the parent env rho and add 1 to it to create a new pseudo-address
    // it will be unique because real pointers are aligned (no odd addresses)
    // info.call_id = make_funcall_id(rho) | 1;

    info.recursion = is_recursive(context, info.fn_id);

    get_stack_parent(info, tracer_state(context).full_stack);
    info.in_prom_id = get_parent_promise(context);

    stack_event_t stack_elem;
    stack_elem.type = stack_type::CALL;
    stack_elem.call_id = info.call_id;
    tracer_state(context).full_stack.push_back(stack_elem);

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
    info.fn_id = get_function_id(context, op);
    info.fn_addr = get_function_addr(op);
    call_stack_elem_t elem = (tracer_state(context).fun_stack.back());
    info.call_id = get<0>(elem);
    if (name != NULL)
        info.name = name;
    info.fn_type = fn_type;
    info.fn_compiled = is_byte_compiled(op);
    info.fn_definition = get_expression(op);

    call_stack_elem_t parent_elem = (tracer_state(context).fun_stack.back());
    info.parent_call_id = get<0>(parent_elem);
    info.recursion = is_recursive(context, info.fn_id);

    info.definition_location = get_definition_location_cpp(op);
    info.callsite_location = get_callsite_cpp(0);

    tracer_state(context).full_stack.pop_back();
    get_stack_parent(info, tracer_state(context).full_stack);
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

    return info;
}

prom_info_t force_promise_entry_get_info(dyntrace_context_t *context,
                                         const SEXP promise) {
    prom_info_t info;
    info.name = "DONT_CARE"; // FIXME WTF is this?!

    info.prom_id = get_promise_id(context, promise);

    call_stack_elem_t call_stack_elem =
        (tracer_state(context).fun_stack.back());
    info.in_call_id = get<0>(call_stack_elem);
    info.from_call_id = tracer_state(context).promise_origin[info.prom_id];

    set_distances_and_lifestyle(context, info);

    info.prom_type = static_cast<sexp_type>(TYPEOF(PRCODE(promise)));
    get_full_type(promise, info.full_type);
    info.return_type = sexp_type::OMEGA;

    get_stack_parent(info, tracer_state(context).full_stack);
    info.in_prom_id = get_parent_promise(context);
    info.depth = get_no_of_ancestor_promises_on_stack(context);

    stack_event_t stack_elem;
    stack_elem.type = stack_type::PROMISE;
    stack_elem.promise_id = info.prom_id;
    tracer_state(context).full_stack.push_back(stack_elem);

    return info;
}

prom_info_t force_promise_exit_get_info(dyntrace_context_t *context,
                                        const SEXP promise) {
    prom_info_t info;
    info.name = "DONT_CARE";

    info.prom_id = get_promise_id(context, promise);

    call_stack_elem_t stack_elem = (tracer_state(context).fun_stack.back());
    info.in_call_id = get<0>(stack_elem);
    info.from_call_id = tracer_state(context).promise_origin[info.prom_id];

    set_distances_and_lifestyle(context, info);

    info.prom_type = static_cast<sexp_type>(TYPEOF(PRCODE(promise)));
    get_full_type(promise, info.full_type);
    info.return_type = static_cast<sexp_type>(TYPEOF(PRVALUE(promise)));

    tracer_state(context).full_stack.pop_back();

    get_stack_parent(info, tracer_state(context).full_stack);
    info.in_prom_id = get_parent_promise(context);
    info.depth = get_no_of_ancestor_promises_on_stack(context);

    return info;
}

prom_info_t promise_lookup_get_info(dyntrace_context_t *context,
                                    const SEXP promise) {
    prom_info_t info;

    info.name = "DONT_CARE";

    info.prom_id = get_promise_id(context, promise);

    call_stack_elem_t stack_elem = (tracer_state(context).fun_stack.back());
    info.in_call_id = get<0>(stack_elem);
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

    call_stack_elem_t stack_elem = (tracer_state(context).fun_stack.back());
    info.in_call_id = get<0>(stack_elem);
    info.from_call_id = tracer_state(context).promise_origin[info.prom_id];

    set_distances_and_lifestyle(context, info);

    info.prom_type = static_cast<sexp_type>(TYPEOF(PRCODE(prom)));
    info.full_type.push_back(sexp_type::OMEGA);
    info.return_type = static_cast<sexp_type>(TYPEOF(PRCODE(prom)));

    return info;
}
