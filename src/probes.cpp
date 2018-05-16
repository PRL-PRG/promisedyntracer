#include "probes.h"
#include "State.h"
#include "Timer.h"

void begin(dyntrace_context_t *context, const SEXP prom) {
    tracer_state(context).start_pass(context, prom);
    debug_serializer(context).serialize_start_trace();
    analysis_driver(context).begin(context);
}

void end(dyntrace_context_t *context) {
    tracer_state(context).finish_pass();

    if (!tracer_state(context).full_stack.empty()) {
        dyntrace_log_warning(
            "Function/promise stack is not balanced: %d remaining",
            tracer_state(context).full_stack.size());
        tracer_state(context).full_stack.clear();
    }

    debug_serializer(context).serialize_finish_trace();
    analysis_driver(context).end(context);
}

// Triggered when entering function evaluation.
void function_entry(dyntrace_context_t *context, const SEXP call, const SEXP op,
                    const SEXP rho) {
    MAIN_TIMER_RESET();

    closure_info_t info = function_entry_get_info(context, call, op, rho);

    MAIN_TIMER_END_SEGMENT(FUNCTION_ENTRY_RECORDER);

    stack_event_t stack_elem;
    stack_elem.type = stack_type::CALL;
    stack_elem.call_id = info.call_id;
    stack_elem.function_info.function_id = info.fn_id;
    stack_elem.function_info.type = function_type::CLOSURE;
    stack_elem.enclosing_environment = info.call_ptr;
    tracer_state(context).full_stack.push_back(stack_elem);

    MAIN_TIMER_END_SEGMENT(FUNCTION_ENTRY_STACK);

    analysis_driver(context).closure_entry(info);

    MAIN_TIMER_END_SEGMENT(FUNCTION_ENTRY_ANALYSIS);

    debug_serializer(context).serialize_function_entry(info);

    auto &fresh_promises = tracer_state(context).fresh_promises;
    // Associate promises with call ID
    for (auto argument : info.arguments) {
        auto &promise = argument.promise_id;
        // if promise environment is same as the caller's environment, then
        // serialize this promise as it is a default argument.

        debug_serializer(context).serialize_promise_argument_type(
            promise, argument.default_argument);

        auto it = fresh_promises.find(promise);
        if (it != fresh_promises.end()) {
            tracer_state(context).promise_origin[promise] = info.call_id;
            fresh_promises.erase(it);
        }
    }

    tracer_serializer(context).serialize_trace(
        TraceSerializer::OPCODE_CLOSURE_BEGIN, info.fn_id, info.call_id,
        tracer_state(context).to_environment_id(rho));

    MAIN_TIMER_END_SEGMENT(FUNCTION_ENTRY_WRITE_TRACE);
}

void function_exit(dyntrace_context_t *context, const SEXP call, const SEXP op,
                   const SEXP rho, const SEXP retval) {

    MAIN_TIMER_RESET();

    closure_info_t info =
        function_exit_get_info(context, call, op, rho, retval);

    MAIN_TIMER_END_SEGMENT(FUNCTION_EXIT_RECORDER);

    auto thing_on_stack = tracer_state(context).full_stack.back();
    if (thing_on_stack.type != stack_type::CALL ||
        thing_on_stack.call_id != info.call_id) {
        dyntrace_log_warning(
            "Object on stack was %s with id %d,"
            " but was expected to be closure with id %d",
            thing_on_stack.type == stack_type::PROMISE ? "promise" : "call",
            thing_on_stack.call_id, info.call_id);
    }
    tracer_state(context).full_stack.pop_back();

    MAIN_TIMER_END_SEGMENT(FUNCTION_EXIT_STACK);

    analysis_driver(context).closure_exit(info);

    MAIN_TIMER_END_SEGMENT(FUNCTION_EXIT_ANALYSIS);

    debug_serializer(context).serialize_function_exit(info);

    tracer_serializer(context).serialize_trace(
        TraceSerializer::OPCODE_CLOSURE_FINISH, info.fn_id, info.call_id,
        tracer_state(context).to_environment_id(rho));

    MAIN_TIMER_END_SEGMENT(FUNCTION_EXIT_WRITE_TRACE);
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
    MAIN_TIMER_RESET();

#ifndef RDT_IGNORE_SPECIALS_AND_BUILTINS
    builtin_info_t info =
        builtin_entry_get_info(context, call, op, rho, fn_type);
#endif

    MAIN_TIMER_END_SEGMENT(BUILTIN_ENTRY_RECORDER);

    if (info.fn_type == function_type::SPECIAL)
        analysis_driver(context).special_entry(info);
    else
        analysis_driver(context).builtin_entry(info);

    MAIN_TIMER_END_SEGMENT(BUILTIN_ENTRY_ANALYSIS);

#ifndef RDT_IGNORE_SPECIALS_AND_BUILTINS
    stack_event_t stack_elem;
    stack_elem.type = stack_type::CALL;
    stack_elem.call_id = info.call_id;
    stack_elem.function_info.function_id = info.fn_id;
    stack_elem.function_info.type = info.fn_type;
    stack_elem.enclosing_environment = info.call_ptr;
    tracer_state(context).full_stack.push_back(stack_elem);
#endif

    MAIN_TIMER_END_SEGMENT(BUILTIN_ENTRY_STACK);

#ifndef RDT_IGNORE_SPECIALS_AND_BUILTINS
    debug_serializer(context).serialize_builtin_entry(info);
#endif

#ifndef RDT_IGNORE_SPECIALS_AND_BUILTINS
    tracer_serializer(context).serialize_trace(
        info.fn_type == function_type::SPECIAL
            ? TraceSerializer::OPCODE_SPECIAL_BEGIN
            : TraceSerializer::OPCODE_BUILTIN_BEGIN,
        info.fn_id, info.call_id, tracer_state(context).to_environment_id(rho));
#endif

    MAIN_TIMER_END_SEGMENT(BUILTIN_ENTRY_WRITE_TRACE);
}

void print_exit_info(dyntrace_context_t *context, const SEXP call,
                     const SEXP op, const SEXP rho, function_type fn_type,
                     const SEXP retval) {
    MAIN_TIMER_RESET();

#ifndef RDT_IGNORE_SPECIALS_AND_BUILTINS
    builtin_info_t info =
        builtin_exit_get_info(context, call, op, rho, fn_type, retval);
#endif

    MAIN_TIMER_END_SEGMENT(BUILTIN_EXIT_STACK);

    if (info.fn_type == function_type::SPECIAL)
        analysis_driver(context).special_exit(info);
    else
        analysis_driver(context).builtin_exit(info);

    MAIN_TIMER_END_SEGMENT(BUILTIN_EXIT_ANALYSIS);

#ifndef RDT_IGNORE_SPECIALS_AND_BUILTINS
    auto thing_on_stack = tracer_state(context).full_stack.back();
    if (thing_on_stack.type != stack_type::CALL ||
        thing_on_stack.call_id != info.call_id) {
        dyntrace_log_warning(
            "Object on stack was %s with id %d,"
            " but was expected to be built-in with id %d",
            thing_on_stack.type == stack_type::PROMISE ? "promise" : "call",
            thing_on_stack.call_id, info.call_id);
    }
    tracer_state(context).full_stack.pop_back();
#endif

    MAIN_TIMER_END_SEGMENT(BUILTIN_EXIT_STACK);

#ifndef RDT_IGNORE_SPECIALS_AND_BUILTINS
    debug_serializer(context).serialize_builtin_exit(info);
#endif

#ifndef RDT_IGNORE_SPECIALS_AND_BUILTINS
    tracer_serializer(context).serialize_trace(
        info.fn_type == function_type::SPECIAL
            ? TraceSerializer::OPCODE_SPECIAL_FINISH
            : TraceSerializer::OPCODE_BUILTIN_FINISH,
        info.fn_id, info.call_id, tracer_state(context).to_environment_id(rho));
#endif

    MAIN_TIMER_END_SEGMENT(BUILTIN_EXIT_WRITE_TRACE);

#ifndef RDT_IGNORE_SPECIALS_AND_BUILTINS
#endif
}

void promise_created(dyntrace_context_t *context, const SEXP prom,
                     const SEXP rho) {
    MAIN_TIMER_RESET();

    prom_basic_info_t info = create_promise_get_info(context, prom, rho);

    MAIN_TIMER_END_SEGMENT(CREATE_PROMISE_RECORDER);

    debug_serializer(context).serialize_promise_created(info);

    if (info.prom_id >= 0) { // maybe we don't need this check
        prom_lifecycle_info_t prom_gc_info = {
            info.prom_id, 0, tracer_state(context).get_gc_trigger_counter()};
        debug_serializer(context).serialize_promise_lifecycle(prom_gc_info, -1);
    }

    analysis_driver(context).promise_created(info, prom);

    MAIN_TIMER_END_SEGMENT(CREATE_PROMISE_ANALYSIS);

    std::string cre_id = std::string("cre ") + std::to_string(info.prom_id);
    debug_serializer(context).serialize_interference_information(cre_id);
    tracer_serializer(context).serialize_trace(
        TraceSerializer::OPCODE_PROMISE_CREATE, info.prom_id,
        tracer_state(context).to_environment_id(PRENV(prom)));

    MAIN_TIMER_END_SEGMENT(CREATE_PROMISE_WRITE_TRACE);
}

// Promise is being used inside a function body for the first time.
void promise_force_entry(dyntrace_context_t *context, const SEXP promise) {
    MAIN_TIMER_RESET();

    prom_info_t info = force_promise_entry_get_info(context, promise);

    MAIN_TIMER_END_SEGMENT(FORCE_PROMISE_ENTRY_RECORDER);

    analysis_driver(context).promise_force_entry(info, promise);

    MAIN_TIMER_END_SEGMENT(FORCE_PROMISE_ENTRY_ANALYSIS);

    stack_event_t stack_elem;
    stack_elem.type = stack_type::PROMISE;
    stack_elem.promise_id = info.prom_id;
    stack_elem.enclosing_environment =
        tracer_state(context)
            .full_stack.back()
            .enclosing_environment; // FIXME necessary?
    tracer_state(context).full_stack.push_back(stack_elem);

    MAIN_TIMER_END_SEGMENT(FORCE_PROMISE_ENTRY_STACK);

    std::string ent_id = std::string("ent ") + std::to_string(info.prom_id);
    debug_serializer(context).serialize_interference_information(ent_id);
    tracer_serializer(context).serialize_trace(
        TraceSerializer::OPCODE_PROMISE_BEGIN, info.prom_id,
        info.expression_id);

    MAIN_TIMER_END_SEGMENT(FORCE_PROMISE_ENTRY_WRITE_TRACE);

    debug_serializer(context).serialize_force_promise_entry(info);
    tracer_state(context).clock_id++;
    /* reset number of environment bindings. We want to know how many bindings
       happened while forcing this promise */
    if (info.prom_id >= 0) {
        prom_lifecycle_info_t prom_gc_info = {
            info.prom_id, 1, tracer_state(context).get_gc_trigger_counter()};
        debug_serializer(context).serialize_promise_lifecycle(prom_gc_info, -1);
    }
}

void promise_force_exit(dyntrace_context_t *context, const SEXP promise) {
    MAIN_TIMER_RESET();

    prom_info_t info = force_promise_exit_get_info(context, promise);

    MAIN_TIMER_END_SEGMENT(FORCE_PROMISE_EXIT_RECORDER);

    analysis_driver(context).promise_force_exit(info, promise);

    MAIN_TIMER_END_SEGMENT(FORCE_PROMISE_EXIT_ANALYSIS);

    auto thing_on_stack = tracer_state(context).full_stack.back();
    if (thing_on_stack.type != stack_type::PROMISE ||
        thing_on_stack.promise_id != info.prom_id) {
        dyntrace_log_warning(
            "Object on stack was %s with id %d,"
            " but was expected to be promise with id %d",
            thing_on_stack.type == stack_type::PROMISE ? "promise" : "call",
            thing_on_stack.promise_id, info.prom_id);
    }
    tracer_state(context).full_stack.pop_back();

    MAIN_TIMER_END_SEGMENT(FORCE_PROMISE_EXIT_STACK);

    std::string ext_id = std::string("ext ") + std::to_string(info.prom_id);
    debug_serializer(context).serialize_interference_information(ext_id);
    tracer_serializer(context).serialize_trace(
        TraceSerializer::OPCODE_PROMISE_FINISH, info.prom_id);

    MAIN_TIMER_END_SEGMENT(FORCE_PROMISE_EXIT_WRITE_TRACE);

    debug_serializer(context).serialize_force_promise_exit(info);

    tracer_state(context).clock_id++;
}

void promise_value_lookup(dyntrace_context_t *context, const SEXP promise,
                          int in_force) {
    MAIN_TIMER_RESET();

    prom_info_t info = promise_lookup_get_info(context, promise);

    analysis_driver(context).promise_value_lookup(info, promise, in_force);

    MAIN_TIMER_END_SEGMENT(LOOKUP_PROMISE_VALUE_ANALYSIS);

    std::string val_id = std::string("val ") + std::to_string(info.prom_id);

    MAIN_TIMER_END_SEGMENT(LOOKUP_PROMISE_VALUE_RECORDER);

    debug_serializer(context).serialize_interference_information(val_id);
    tracer_serializer(context).serialize_trace(
        TraceSerializer::OPCODE_PROMISE_VALUE, info.prom_id);

    MAIN_TIMER_END_SEGMENT(LOOKUP_PROMISE_VALUE_WRITE_TRACE);

    if (info.prom_id >= 0) {
        debug_serializer(context).serialize_promise_lookup(info);
        tracer_state(context).clock_id++;
        prom_lifecycle_info_t prom_gc_info{
            info.prom_id, 5, tracer_state(context).gc_trigger_counter};
        debug_serializer(context).serialize_promise_lifecycle(prom_gc_info,
                                                              in_force);
    }
}

void promise_expression_lookup(dyntrace_context_t *context, const SEXP prom,
                               int in_force) {
    MAIN_TIMER_RESET();
    prom_info_t info = promise_expression_lookup_get_info(context, prom);

    MAIN_TIMER_END_SEGMENT(LOOKUP_PROMISE_EXPRESSION_RECORDER);

    analysis_driver(context).promise_expression_lookup(info, prom, in_force);

    MAIN_TIMER_END_SEGMENT(LOOKUP_PROMISE_EXPRESSION_ANALYSIS);

    if (info.prom_id >= 0) {
        tracer_serializer(context).serialize_trace(
            TraceSerializer::OPCODE_PROMISE_EXPRESSION, info.prom_id);

        MAIN_TIMER_END_SEGMENT(LOOKUP_PROMISE_EXPRESSION_WRITE_TRACE);

        tracer_state(context).clock_id++;
        prom_lifecycle_info_t prom_gc_info = {
            info.prom_id, 3, tracer_state(context).get_gc_trigger_counter()};

        debug_serializer(context).serialize_promise_lifecycle(prom_gc_info,
                                                              in_force);
    }
}

void promise_environment_lookup(dyntrace_context_t *context, const SEXP prom,
                                int in_force) {
    MAIN_TIMER_RESET();

    prom_info_t info = promise_expression_lookup_get_info(context, prom);

    MAIN_TIMER_END_SEGMENT(LOOKUP_PROMISE_ENVIRONMENT_RECORDER);

    analysis_driver(context).promise_environment_lookup(info, prom, in_force);

    MAIN_TIMER_END_SEGMENT(LOOKUP_PROMISE_ENVIRONMENT_ANALYSIS);

    if (info.prom_id >= 0) {
        tracer_serializer(context).serialize_trace(
            TraceSerializer::OPCODE_PROMISE_ENVIRONMENT, info.prom_id);

        MAIN_TIMER_END_SEGMENT(LOOKUP_PROMISE_ENVIRONMENT_WRITE_TRACE);

        // tracer_serializer(context).serialize_promise_environment_lookup(
        //         info, tracer_state(context).clock_id); // FIXME
        tracer_state(context).clock_id++;
        prom_lifecycle_info_t prom_gc_info = {
            info.prom_id, 4, tracer_state(context).get_gc_trigger_counter()};

        debug_serializer(context).serialize_promise_lifecycle(prom_gc_info,
                                                              in_force);
    }
}

// void promise_value_lookup(dyntrace_context_t *context, const SEXP prom, int
// in_force) {
//    prom_info_t info = promise_expression_lookup_get_info(context, prom);
//    if (info.prom_id >= 0) {
//        tracer_serializer(context).serialize_trace(TraceSerializer::OPCODE_PROMISE_ENVIRONMENT,
//                                                   info.prom_id);
//        //tracer_serializer(context).serialize_promise_environment_lookup(
//        //         info, tracer_state(context).clock_id); // FIXME
//        tracer_state(context).clock_id++;
//        prom_lifecycle_info_t prom_gc_info = {
//                info.prom_id,
//                5,
//                tracer_state(context).get_gc_trigger_counter(),
//                tracer_state(context).get_builtin_counter(),
//                tracer_state(context).get_special_counter(),
//                tracer_state(context).get_closure_counter()};
//        debug_serializer(context).serialize_promise_lifecycle(prom_gc_info,
//        in_force);
//        tracer_serializer(context).serialize_promise_lifecycle(prom_gc_info,
//        in_force);
//    }
//}

void promise_expression_set(dyntrace_context_t *context, const SEXP prom,
                            int in_force) {
    MAIN_TIMER_RESET();

    prom_info_t info = promise_expression_lookup_get_info(context, prom);

    MAIN_TIMER_END_SEGMENT(SET_PROMISE_EXPRESSION_RECORDER);

    analysis_driver(context).promise_expression_set(info, prom, in_force);

    MAIN_TIMER_END_SEGMENT(SET_PROMISE_EXPRESSION_ANALYSIS);

    if (info.prom_id >= 0) {
        tracer_serializer(context).serialize_trace(
            TraceSerializer::OPCODE_PROMISE_ENVIRONMENT, info.prom_id);

        MAIN_TIMER_END_SEGMENT(SET_PROMISE_EXPRESSION_WRITE_TRACE);

        // tracer_serializer(context).serialize_promise_environment_lookup(
        //         info, tracer_state(context).clock_id); // FIXME
        tracer_state(context).clock_id++;
        prom_lifecycle_info_t prom_gc_info = {
            info.prom_id, 6, tracer_state(context).get_gc_trigger_counter()};

        debug_serializer(context).serialize_promise_lifecycle(prom_gc_info,
                                                              in_force);
    }
}

void promise_value_set(dyntrace_context_t *context, const SEXP prom,
                       int in_force) {
    MAIN_TIMER_RESET();

    prom_info_t info = promise_expression_lookup_get_info(context, prom);

    MAIN_TIMER_END_SEGMENT(SET_PROMISE_VALUE_RECORDER);

    analysis_driver(context).promise_value_set(info, prom, in_force);

    MAIN_TIMER_END_SEGMENT(SET_PROMISE_VALUE_ANALYSIS);

    if (info.prom_id >= 0) {
        tracer_serializer(context).serialize_trace(
            TraceSerializer::OPCODE_PROMISE_ENVIRONMENT, info.prom_id);
        // tracer_serializer(context).serialize_promise_environment_lookup(
        //         info, tracer_state(context).clock_id); // FIXME
        tracer_state(context).clock_id++;
        prom_lifecycle_info_t prom_gc_info = {
            info.prom_id, 8, tracer_state(context).get_gc_trigger_counter()};

        debug_serializer(context).serialize_promise_lifecycle(prom_gc_info,
                                                              in_force);
    }
}

void promise_environment_set(dyntrace_context_t *context, const SEXP prom,
                             int in_force) {
    MAIN_TIMER_RESET();

    prom_info_t info = promise_expression_lookup_get_info(context, prom);

    MAIN_TIMER_END_SEGMENT(SET_PROMISE_ENVIRONMENT_RECORDER);

    analysis_driver(context).promise_environment_set(info, prom, in_force);

    MAIN_TIMER_END_SEGMENT(SET_PROMISE_ENVIRONMENT_ANALYSIS);

    if (info.prom_id >= 0) {
        tracer_serializer(context).serialize_trace(
            TraceSerializer::OPCODE_PROMISE_ENVIRONMENT, info.prom_id);
        // tracer_serializer(context).serialize_promise_environment_lookup(
        //         info, tracer_state(context).clock_id); // FIXME
        tracer_state(context).clock_id++;
        prom_lifecycle_info_t prom_gc_info = {
            info.prom_id, 7, tracer_state(context).get_gc_trigger_counter()};

        debug_serializer(context).serialize_promise_lifecycle(prom_gc_info,
                                                              in_force);
    }
}

void gc_promise_unmarked(dyntrace_context_t *context, const SEXP promise) {
    MAIN_TIMER_RESET();

    prom_addr_t addr = get_sexp_address(promise);
    prom_id_t id = get_promise_id(context, promise);
    auto &promise_origin = tracer_state(context).promise_origin;

    MAIN_TIMER_END_SEGMENT(GC_PROMISE_UNMARKED_RECORDER);

    if (id >= 0) {
        prom_lifecycle_info_t prom_gc_info = {
            id, 2, tracer_state(context).get_gc_trigger_counter()};

        debug_serializer(context).serialize_promise_lifecycle(prom_gc_info, -1);
    }

    analysis_driver(context).gc_promise_unmarked(id, promise);

    MAIN_TIMER_END_SEGMENT(GC_PROMISE_UNMARKED_ANALYSIS);

    auto iter = promise_origin.find(id);
    if (iter != promise_origin.end()) {
        // If this is one of our traced promises,
        // delete it from origin map because it is ready to be GCed
        promise_origin.erase(iter);
    }

    unsigned int prom_type = TYPEOF(PRCODE(promise));
    unsigned int orig_type =
        (prom_type == 21) ? TYPEOF(BODY_EXPR(PRCODE(promise))) : 0;
    prom_key_t key(addr, prom_type, orig_type);
    tracer_state(context).promise_ids.erase(key);

    MAIN_TIMER_END_SEGMENT(GC_PROMISE_UNMARKED_RECORD_KEEPING);
}

void gc_function_unmarked(dyntrace_context_t *context, const SEXP function) {
    MAIN_TIMER_RESET();

    remove_function_definition(context, function);

    MAIN_TIMER_END_SEGMENT(GC_FUNCTION_UNMARKED_RECORD_KEEPING);
}

void gc_entry(dyntrace_context_t *context, R_size_t size_needed) {
    MAIN_TIMER_RESET();

    tracer_state(context).increment_gc_trigger_counter();

    MAIN_TIMER_END_SEGMENT(GC_ENTRY_RECORDER);
}

void gc_exit(dyntrace_context_t *context, int gc_count, double vcells,
             double ncells) {
    MAIN_TIMER_RESET();

    gc_info_t info;
    info.vcells = vcells;
    info.ncells = ncells;
    info.counter = tracer_state(context).get_gc_trigger_counter();

    MAIN_TIMER_END_SEGMENT(GC_EXIT_RECORDER);

    debug_serializer(context).serialize_gc_exit(info);
}

void vector_alloc(dyntrace_context_t *context, int sexptype, long length,
                  long bytes, const char *srcref) {
    MAIN_TIMER_RESET();

    type_gc_info_t info{tracer_state(context).get_gc_trigger_counter(),
                        sexptype, length, bytes};

    MAIN_TIMER_END_SEGMENT(VECTOR_ALLOC_RECORDER);

    debug_serializer(context).serialize_vector_alloc(info);

    analysis_driver(context).vector_alloc(info);

    MAIN_TIMER_END_SEGMENT(VECTOR_ALLOC_ANALYSIS);
}

void new_environment(dyntrace_context_t *context, const SEXP rho) {
    MAIN_TIMER_RESET();

    // fn_id_t fn_id = (tracer_state(context).fun_stack.back().function_id);
    stack_event_t event = get_last_on_stack_by_type(
        tracer_state(context).full_stack, stack_type::CALL);
    fn_id_t fn_id = event.type == stack_type::NONE
                        ? compute_hash("")
                        : event.function_info.function_id;

    env_id_t env_id = tracer_state(context).environment_id_counter++;

    MAIN_TIMER_END_SEGMENT(NEW_ENVIRONMENT_RECORDER);

    debug_serializer(context).serialize_new_environment(env_id, fn_id);
    tracer_state(context).environments[rho] =
        std::pair<env_id_t, unordered_map<string, var_id_t>>(env_id, {});

    tracer_serializer(context).serialize_trace(
        TraceSerializer::OPCODE_ENVIRONMENT_CREATE, env_id);

    MAIN_TIMER_END_SEGMENT(NEW_ENVIRONMENT_WRITE_TRACE);
}

void begin_ctxt(dyntrace_context_t *context, const RCNTXT *cptr) {
    MAIN_TIMER_RESET();

    stack_event_t event;
    event.context_id = (rid_t)cptr;
    event.type = stack_type::CONTEXT;
    tracer_state(context).full_stack.push_back(event);
    debug_serializer(context).serialize_begin_ctxt(cptr);

    MAIN_TIMER_END_SEGMENT(CONTEXT_ENTRY_STACK);
}

void jump_ctxt(dyntrace_context_t *context, const RCNTXT *cptr,
               const SEXP return_value, int restart) {
    MAIN_TIMER_RESET();

    unwind_info_t info;
    info.jump_context = ((rid_t)cptr);
    info.restart = restart;
    adjust_stacks(context, info);
    MAIN_TIMER_END_SEGMENT(CONTEXT_JUMP_STACK);

    debug_serializer(context).serialize_unwind(info);
}

void end_ctxt(dyntrace_context_t *context, const RCNTXT *cptr) {
    MAIN_TIMER_RESET();

    stack_event_t event = tracer_state(context).full_stack.back();
    if (event.type == stack_type::CONTEXT && ((rid_t)cptr) == event.context_id)
        tracer_state(context).full_stack.pop_back();
    else
        dyntrace_log_warning("Context trying to remove context %d from full "
                             "stack, but %d is on top of stack.",
                             ((rid_t)cptr), event.context_id);
    debug_serializer(context).serialize_end_ctxt(cptr);

    MAIN_TIMER_END_SEGMENT(CONTEXT_EXIT_STACK);
}

void adjust_stacks(dyntrace_context_t *context, unwind_info_t &info) {
    while (!tracer_state(context).full_stack.empty()) {
        stack_event_t event_from_fullstack =
            (tracer_state(context).full_stack.back());

        // if (info.jump_target == event_from_fullstack.enclosing_environment)
        //    break;
        if (event_from_fullstack.type == stack_type::CONTEXT)
            if (info.jump_context == event_from_fullstack.context_id)
                break;
            else
                info.unwound_contexts.push_back(
                    event_from_fullstack.context_id);
        else if (event_from_fullstack.type == stack_type::CALL) {
            tracer_serializer(context).serialize_trace(
                TraceSerializer::OPCODE_FUNCTION_CONTEXT_JUMP,
                event_from_fullstack.call_id);
            info.unwound_calls.push_back(event_from_fullstack.call_id);
        } else if (event_from_fullstack.type == stack_type::PROMISE) {
            tracer_serializer(context).serialize_trace(
                TraceSerializer::OPCODE_PROMISE_CONTEXT_JUMP,
                event_from_fullstack.promise_id);
            info.unwound_promises.push_back(event_from_fullstack.promise_id);
        } else /* if (event_from_fullstack.type == stack_type::NONE) */
            dyntrace_log_error("NONE object found on tracer's full stack.");

        tracer_state(context).full_stack.pop_back();
    }
}

void environment_action(dyntrace_context_t *context, const SEXP symbol,
                        SEXP value, const SEXP rho, const std::string &action) {
    bool exists = true;
    prom_id_t promise_id = tracer_state(context).enclosing_promise_id();
    env_id_t environment_id = tracer_state(context).to_environment_id(rho);
    var_id_t variable_id =
        tracer_state(context).to_variable_id(symbol, rho, exists);
    // std::string value = serialize_sexp(expression);
    // std::string exphash = compute_hash(value.c_str());
    // serialize variable iff it has not been seen before.
    // if it has been seen before, then it has already been serialized.

    MAIN_TIMER_END_SEGMENT(ENVIRONMENT_ACTION_RECORDER);

    if (!exists) {
        debug_serializer(context).serialize_variable(
            variable_id, CHAR(PRINTNAME(symbol)), environment_id);
    }

    debug_serializer(context).serialize_variable_action(promise_id, variable_id,
                                                        action);

    std::string action_id = action + " " + std::to_string(variable_id);
    debug_serializer(context).serialize_interference_information(action_id);
    tracer_serializer(context).serialize_trace(
        action, tracer_state(context).to_environment_id(rho), variable_id,
        CHAR(PRINTNAME(symbol)), sexp_type_to_string((sexp_type)TYPEOF(value)));

    MAIN_TIMER_END_SEGMENT(ENVIRONMENT_ACTION_WRITE_TRACE);
}

void environment_define_var(dyntrace_context_t *context, const SEXP symbol,
                            const SEXP value, const SEXP rho) {
    MAIN_TIMER_RESET();

    analysis_driver(context).environment_define_var(symbol, value, rho);

    MAIN_TIMER_END_SEGMENT(ENVIRONMENT_ACTION_ANALYSIS);

    environment_action(context, symbol, value, rho,
                       TraceSerializer::OPCODE_ENVIRONMENT_DEFINE);
}

void environment_assign_var(dyntrace_context_t *context, const SEXP symbol,
                            const SEXP value, const SEXP rho) {
    MAIN_TIMER_RESET();

    analysis_driver(context).environment_assign_var(symbol, value, rho);

    MAIN_TIMER_END_SEGMENT(ENVIRONMENT_ACTION_ANALYSIS);

    environment_action(context, symbol, value, rho,
                       TraceSerializer::OPCODE_ENVIRONMENT_ASSIGN);
}

void environment_remove_var(dyntrace_context_t *context, const SEXP symbol,
                            const SEXP rho) {
    MAIN_TIMER_RESET();

    analysis_driver(context).environment_remove_var(symbol, rho);

    MAIN_TIMER_END_SEGMENT(ENVIRONMENT_ACTION_ANALYSIS);

    environment_action(context, symbol, R_UnboundValue, rho,
                       TraceSerializer::OPCODE_ENVIRONMENT_REMOVE);
}

void environment_lookup_var(dyntrace_context_t *context, const SEXP symbol,
                            const SEXP value, const SEXP rho) {
    MAIN_TIMER_RESET();

    analysis_driver(context).environment_lookup_var(symbol, value, rho);

    MAIN_TIMER_END_SEGMENT(ENVIRONMENT_ACTION_ANALYSIS);

    environment_action(context, symbol, value, rho,
                       TraceSerializer::OPCODE_ENVIRONMENT_LOOKUP);
}
