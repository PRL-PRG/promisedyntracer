#include "probes.h"
#include "State.h"
#include "Timer.h"

void dyntrace_entry(dyntracer_t *dyntracer, SEXP expression, SEXP environment) {
    MAIN_TIMER_RESET();

    debug_serializer(dyntracer).serialize_start_trace();

    MAIN_TIMER_END_SEGMENT(BEGIN_SETUP);

    analysis_driver(dyntracer).begin(dyntracer);

    MAIN_TIMER_END_SEGMENT(BEGIN_ANALYSIS);
}

void dyntrace_exit(dyntracer_t *dyntracer, SEXP expression, SEXP environment,
                   SEXP result, int error) {
    MAIN_TIMER_RESET();

    tracer_state(dyntracer).finish_pass();

    if (!tracer_state(dyntracer).full_stack.empty()) {
        dyntrace_log_warning(
            "Function/promise stack is not balanced: %d remaining",
            tracer_state(dyntracer).full_stack.size());
        tracer_state(dyntracer).full_stack.clear();
    }

    debug_serializer(dyntracer).serialize_finish_trace();

    MAIN_TIMER_END_SEGMENT(END_CHECK);

    analysis_driver(dyntracer).end(dyntracer);

    MAIN_TIMER_END_SEGMENT(END_ANALYSIS);
}

// Triggered when entering function evaluation.
void closure_entry(dyntracer_t *dyntracer, const SEXP call, const SEXP op,
                   const SEXP args, const SEXP rho) {
    MAIN_TIMER_RESET();

    closure_info_t info = function_entry_get_info(dyntracer, call, op, rho);

    MAIN_TIMER_END_SEGMENT(FUNCTION_ENTRY_RECORDER);

    stack_event_t stack_elem;
    stack_elem.type = stack_type::CALL;
    stack_elem.call_id = info.call_id;
    stack_elem.function_info.function_id = info.fn_id;
    stack_elem.function_info.type = function_type::CLOSURE;
    stack_elem.enclosing_environment = info.call_ptr;
    tracer_state(dyntracer).full_stack.push_back(stack_elem);

    MAIN_TIMER_END_SEGMENT(FUNCTION_ENTRY_STACK);

    analysis_driver(dyntracer).closure_entry(info);

    MAIN_TIMER_END_SEGMENT(FUNCTION_ENTRY_ANALYSIS);

    debug_serializer(dyntracer).serialize_function_entry(info);

    auto &fresh_promises = tracer_state(dyntracer).fresh_promises;
    // Associate promises with call ID
    for (auto argument : info.arguments) {
        auto &promise = argument.promise_id;
        // if promise environment is same as the caller's environment, then
        // serialize this promise as it is a default argument.

        debug_serializer(dyntracer).serialize_promise_argument_type(
            promise, argument.default_argument);

        auto it = fresh_promises.find(promise);
        if (it != fresh_promises.end()) {
            tracer_state(dyntracer).promise_origin[promise] = info.call_id;
            fresh_promises.erase(it);
        }
    }

    tracer_serializer(dyntracer).serialize(
        TraceSerializer::OPCODE_FUNCTION_BEGIN, "closure", info.fn_id,
        info.call_id, tracer_state(dyntracer).to_environment_id(rho));

    MAIN_TIMER_END_SEGMENT(FUNCTION_ENTRY_WRITE_TRACE);
}

void closure_exit(dyntracer_t *dyntracer, const SEXP call, const SEXP op,
                  const SEXP args, const SEXP rho, const SEXP retval) {

    MAIN_TIMER_RESET();

    closure_info_t info =
        function_exit_get_info(dyntracer, call, op, rho, retval);

    MAIN_TIMER_END_SEGMENT(FUNCTION_EXIT_RECORDER);

    auto thing_on_stack = tracer_state(dyntracer).full_stack.back();
    if (thing_on_stack.type != stack_type::CALL ||
        thing_on_stack.call_id != info.call_id) {
        dyntrace_log_warning(
            "Object on stack was %s with id %d,"
            " but was expected to be closure with id %d",
            thing_on_stack.type == stack_type::PROMISE ? "promise" : "call",
            thing_on_stack.call_id, info.call_id);
    }
    tracer_state(dyntracer).full_stack.pop_back();

    MAIN_TIMER_END_SEGMENT(FUNCTION_EXIT_STACK);

    analysis_driver(dyntracer).closure_exit(info);

    MAIN_TIMER_END_SEGMENT(FUNCTION_EXIT_ANALYSIS);

    debug_serializer(dyntracer).serialize_function_exit(info);

    tracer_serializer(dyntracer).serialize(
        TraceSerializer::OPCODE_FUNCTION_FINISH, "closure", info.fn_id,
        info.call_id, tracer_state(dyntracer).to_environment_id(rho));

    MAIN_TIMER_END_SEGMENT(FUNCTION_EXIT_WRITE_TRACE);
}

void builtin_entry(dyntracer_t *dyntracer, const SEXP call, const SEXP op,
                   const SEXP args, const SEXP rho) {

    function_type fn_type;
    if (TYPEOF(op) == BUILTINSXP)
        fn_type = (PRIMINTERNAL(op) == 0) ? function_type::TRUE_BUILTIN
                                          : function_type::BUILTIN;
    else /*the weird case of NewBuiltin2 , where op is a language expression*/
        fn_type = function_type::TRUE_BUILTIN;
    print_entry_info(dyntracer, call, op, args, rho, fn_type);
}

void builtin_exit(dyntracer_t *dyntracer, const SEXP call, const SEXP op,
                  const SEXP args, const SEXP rho, const SEXP retval) {
    function_type fn_type;
    if (TYPEOF(op) == BUILTINSXP)
        fn_type = (PRIMINTERNAL(op) == 0) ? function_type::TRUE_BUILTIN
                                          : function_type::BUILTIN;
    else
        fn_type = function_type::TRUE_BUILTIN;
    print_exit_info(dyntracer, call, op, args, rho, fn_type, retval);
}

void special_entry(dyntracer_t *dyntracer, const SEXP call, const SEXP op,
                   const SEXP args, const SEXP rho) {

    print_entry_info(dyntracer, call, op, args, rho, function_type::SPECIAL);
}

void special_exit(dyntracer_t *dyntracer, const SEXP call, const SEXP op,
                  const SEXP args, const SEXP rho, const SEXP retval) {
    print_exit_info(dyntracer, call, op, args, rho, function_type::SPECIAL,
                    retval);
}

void print_entry_info(dyntracer_t *dyntracer, const SEXP call, const SEXP op,
                      const SEXP args, const SEXP rho, function_type fn_type) {
    MAIN_TIMER_RESET();

#ifndef RDT_IGNORE_SPECIALS_AND_BUILTINS
    builtin_info_t info =
        builtin_entry_get_info(dyntracer, call, op, rho, fn_type);
#endif

    MAIN_TIMER_END_SEGMENT(BUILTIN_ENTRY_RECORDER);

    if (info.fn_type == function_type::SPECIAL)
        analysis_driver(dyntracer).special_entry(info);
    else
        analysis_driver(dyntracer).builtin_entry(info);

    MAIN_TIMER_END_SEGMENT(BUILTIN_ENTRY_ANALYSIS);

#ifndef RDT_IGNORE_SPECIALS_AND_BUILTINS
    stack_event_t stack_elem;
    stack_elem.type = stack_type::CALL;
    stack_elem.call_id = info.call_id;
    stack_elem.function_info.function_id = info.fn_id;
    stack_elem.function_info.type = info.fn_type;
    stack_elem.enclosing_environment = info.call_ptr;
    tracer_state(dyntracer).full_stack.push_back(stack_elem);
#endif

    MAIN_TIMER_END_SEGMENT(BUILTIN_ENTRY_STACK);

#ifndef RDT_IGNORE_SPECIALS_AND_BUILTINS
    debug_serializer(dyntracer).serialize_builtin_entry(info);
#endif

#ifndef RDT_IGNORE_SPECIALS_AND_BUILTINS
    tracer_serializer(dyntracer).serialize(
        TraceSerializer::OPCODE_FUNCTION_BEGIN,
        info.fn_type == function_type::SPECIAL ? "special" : "builtin",
        info.fn_id, info.call_id,
        tracer_state(dyntracer).to_environment_id(rho));
#endif

    MAIN_TIMER_END_SEGMENT(BUILTIN_ENTRY_WRITE_TRACE);
}

void print_exit_info(dyntracer_t *dyntracer, const SEXP call, const SEXP op,
                     const SEXP args, const SEXP rho, function_type fn_type,
                     const SEXP retval) {
    MAIN_TIMER_RESET();

#ifndef RDT_IGNORE_SPECIALS_AND_BUILTINS
    builtin_info_t info =
        builtin_exit_get_info(dyntracer, call, op, rho, fn_type, retval);
#endif

    MAIN_TIMER_END_SEGMENT(BUILTIN_EXIT_STACK);

    if (info.fn_type == function_type::SPECIAL)
        analysis_driver(dyntracer).special_exit(info);
    else
        analysis_driver(dyntracer).builtin_exit(info);

    MAIN_TIMER_END_SEGMENT(BUILTIN_EXIT_ANALYSIS);

#ifndef RDT_IGNORE_SPECIALS_AND_BUILTINS
    auto thing_on_stack = tracer_state(dyntracer).full_stack.back();
    if (thing_on_stack.type != stack_type::CALL ||
        thing_on_stack.call_id != info.call_id) {
        dyntrace_log_warning(
            "Object on stack was %s with id %d,"
            " but was expected to be built-in with id %d",
            thing_on_stack.type == stack_type::PROMISE ? "promise" : "call",
            thing_on_stack.call_id, info.call_id);
    }
    tracer_state(dyntracer).full_stack.pop_back();
#endif

    MAIN_TIMER_END_SEGMENT(BUILTIN_EXIT_STACK);

#ifndef RDT_IGNORE_SPECIALS_AND_BUILTINS
    debug_serializer(dyntracer).serialize_builtin_exit(info);
#endif

#ifndef RDT_IGNORE_SPECIALS_AND_BUILTINS
    tracer_serializer(dyntracer).serialize(
        TraceSerializer::OPCODE_FUNCTION_FINISH,
        info.fn_type == function_type::SPECIAL ? "special" : "builtin",
        info.fn_id, info.call_id,
        tracer_state(dyntracer).to_environment_id(rho));
#endif

    MAIN_TIMER_END_SEGMENT(BUILTIN_EXIT_WRITE_TRACE);

#ifndef RDT_IGNORE_SPECIALS_AND_BUILTINS
#endif
}

void gc_allocate(dyntracer_t *dyntracer, const SEXP object) {
    switch (TYPEOF(object)) {
        case PROMSXP:
            return promise_created(dyntracer, object);
        case ENVSXP:
            return new_environment(dyntracer, object);
    }
    if (isVector(object))
        return vector_alloc(dyntracer, TYPEOF(object), xlength(object),
                            BYTE2VEC(xlength(object)), NULL);
}

void promise_created(dyntracer_t *dyntracer, const SEXP prom) {
    MAIN_TIMER_RESET();

    const SEXP rho = dyntrace_get_promise_environment(prom);

    prom_basic_info_t info = create_promise_get_info(dyntracer, prom, rho);

    MAIN_TIMER_END_SEGMENT(CREATE_PROMISE_RECORDER);

    debug_serializer(dyntracer).serialize_promise_created(info);

    analysis_driver(dyntracer).promise_created(info, prom);

    MAIN_TIMER_END_SEGMENT(CREATE_PROMISE_ANALYSIS);

    std::string cre_id = std::string("cre ") + std::to_string(info.prom_id);
    debug_serializer(dyntracer).serialize_interference_information(cre_id);
    tracer_serializer(dyntracer).serialize(
        TraceSerializer::OPCODE_PROMISE_CREATE, info.prom_id,
        tracer_state(dyntracer).to_environment_id(PRENV(prom)),
        info.expression);

    MAIN_TIMER_END_SEGMENT(CREATE_PROMISE_WRITE_TRACE);
}

// Promise is being used inside a function body for the first time.
void promise_force_entry(dyntracer_t *dyntracer, const SEXP promise) {
    MAIN_TIMER_RESET();

    prom_info_t info = force_promise_entry_get_info(dyntracer, promise);

    MAIN_TIMER_END_SEGMENT(FORCE_PROMISE_ENTRY_RECORDER);

    analysis_driver(dyntracer).promise_force_entry(info, promise);

    MAIN_TIMER_END_SEGMENT(FORCE_PROMISE_ENTRY_ANALYSIS);

    stack_event_t stack_elem;
    stack_elem.type = stack_type::PROMISE;
    stack_elem.promise_id = info.prom_id;
    stack_elem.enclosing_environment =
        tracer_state(dyntracer)
            .full_stack.back()
            .enclosing_environment; // FIXME necessary?
    tracer_state(dyntracer).full_stack.push_back(stack_elem);

    MAIN_TIMER_END_SEGMENT(FORCE_PROMISE_ENTRY_STACK);

    std::string ent_id = std::string("ent ") + std::to_string(info.prom_id);
    debug_serializer(dyntracer).serialize_interference_information(ent_id);
    tracer_serializer(dyntracer).serialize(
        TraceSerializer::OPCODE_PROMISE_BEGIN, info.prom_id);

    MAIN_TIMER_END_SEGMENT(FORCE_PROMISE_ENTRY_WRITE_TRACE);

    debug_serializer(dyntracer).serialize_force_promise_entry(info);
}

void promise_force_exit(dyntracer_t *dyntracer, const SEXP promise) {
    MAIN_TIMER_RESET();

    prom_info_t info = force_promise_exit_get_info(dyntracer, promise);

    MAIN_TIMER_END_SEGMENT(FORCE_PROMISE_EXIT_RECORDER);

    analysis_driver(dyntracer).promise_force_exit(info, promise);

    MAIN_TIMER_END_SEGMENT(FORCE_PROMISE_EXIT_ANALYSIS);

    auto thing_on_stack = tracer_state(dyntracer).full_stack.back();
    if (thing_on_stack.type != stack_type::PROMISE ||
        thing_on_stack.promise_id != info.prom_id) {
        dyntrace_log_warning(
            "Object on stack was %s with id %d,"
            " but was expected to be promise with id %d",
            thing_on_stack.type == stack_type::PROMISE ? "promise" : "call",
            thing_on_stack.promise_id, info.prom_id);
    }
    tracer_state(dyntracer).full_stack.pop_back();

    MAIN_TIMER_END_SEGMENT(FORCE_PROMISE_EXIT_STACK);

    std::string ext_id = std::string("ext ") + std::to_string(info.prom_id);
    debug_serializer(dyntracer).serialize_interference_information(ext_id);
    tracer_serializer(dyntracer).serialize(
        TraceSerializer::OPCODE_PROMISE_FINISH, info.prom_id);

    MAIN_TIMER_END_SEGMENT(FORCE_PROMISE_EXIT_WRITE_TRACE);

    debug_serializer(dyntracer).serialize_force_promise_exit(info);
}

void promise_value_lookup(dyntracer_t *dyntracer, const SEXP promise) {
    MAIN_TIMER_RESET();

    prom_info_t info = promise_lookup_get_info(dyntracer, promise);

    analysis_driver(dyntracer).promise_value_lookup(info, promise);

    MAIN_TIMER_END_SEGMENT(LOOKUP_PROMISE_VALUE_ANALYSIS);

    std::string val_id = std::string("val ") + std::to_string(info.prom_id);

    MAIN_TIMER_END_SEGMENT(LOOKUP_PROMISE_VALUE_RECORDER);

    debug_serializer(dyntracer).serialize_interference_information(val_id);

    tracer_serializer(dyntracer).serialize(
        TraceSerializer::OPCODE_PROMISE_VALUE_LOOKUP, info.prom_id,
        value_type_to_string(dyntrace_get_promise_value(promise)));

    MAIN_TIMER_END_SEGMENT(LOOKUP_PROMISE_VALUE_WRITE_TRACE);
}

void promise_expression_lookup(dyntracer_t *dyntracer, const SEXP prom) {
    MAIN_TIMER_RESET();
    prom_info_t info = promise_expression_lookup_get_info(dyntracer, prom);

    MAIN_TIMER_END_SEGMENT(LOOKUP_PROMISE_EXPRESSION_RECORDER);

    analysis_driver(dyntracer).promise_expression_lookup(info, prom);

    MAIN_TIMER_END_SEGMENT(LOOKUP_PROMISE_EXPRESSION_ANALYSIS);

    tracer_serializer(dyntracer).serialize(
        TraceSerializer::OPCODE_PROMISE_EXPRESSION_LOOKUP, info.prom_id,
        get_expression(dyntrace_get_promise_expression(prom)));

    MAIN_TIMER_END_SEGMENT(LOOKUP_PROMISE_EXPRESSION_WRITE_TRACE);
}

void promise_environment_lookup(dyntracer_t *dyntracer, const SEXP prom) {
    MAIN_TIMER_RESET();

    prom_info_t info = promise_expression_lookup_get_info(dyntracer, prom);

    auto environment_id{tracer_state(dyntracer).to_environment_id(
        dyntrace_get_promise_environment(prom))};

    MAIN_TIMER_END_SEGMENT(LOOKUP_PROMISE_ENVIRONMENT_RECORDER);

    analysis_driver(dyntracer).promise_environment_lookup(info, prom);

    MAIN_TIMER_END_SEGMENT(LOOKUP_PROMISE_ENVIRONMENT_ANALYSIS);

    tracer_serializer(dyntracer).serialize(
        TraceSerializer::OPCODE_PROMISE_ENVIRONMENT_LOOKUP, info.prom_id,
        environment_id);

    MAIN_TIMER_END_SEGMENT(LOOKUP_PROMISE_ENVIRONMENT_WRITE_TRACE);
}

void promise_expression_assign(dyntracer_t *dyntracer, const SEXP prom,
                               const SEXP expression) {
    MAIN_TIMER_RESET();

    prom_info_t info = promise_expression_lookup_get_info(dyntracer, prom);

    MAIN_TIMER_END_SEGMENT(SET_PROMISE_EXPRESSION_RECORDER);

    analysis_driver(dyntracer).promise_expression_set(info, prom);

    MAIN_TIMER_END_SEGMENT(SET_PROMISE_EXPRESSION_ANALYSIS);

    tracer_serializer(dyntracer).serialize(
        TraceSerializer::OPCODE_PROMISE_EXPRESSION_ASSIGN, info.prom_id,
        get_expression(expression));

    MAIN_TIMER_END_SEGMENT(SET_PROMISE_EXPRESSION_WRITE_TRACE);
}

void promise_value_assign(dyntracer_t *dyntracer, const SEXP prom,
                          const SEXP value) {
    MAIN_TIMER_RESET();

    prom_info_t info = promise_expression_lookup_get_info(dyntracer, prom);

    MAIN_TIMER_END_SEGMENT(SET_PROMISE_VALUE_RECORDER);

    analysis_driver(dyntracer).promise_value_set(info, prom);

    MAIN_TIMER_END_SEGMENT(SET_PROMISE_VALUE_ANALYSIS);

    tracer_serializer(dyntracer).serialize(
        TraceSerializer::OPCODE_PROMISE_VALUE_ASSIGN, info.prom_id,
        value_type_to_string(value));
}

void promise_environment_assign(dyntracer_t *dyntracer, const SEXP prom,
                                const SEXP environment) {

    MAIN_TIMER_RESET();

    prom_info_t info = promise_expression_lookup_get_info(dyntracer, prom);
    auto environment_id =
        tracer_state(dyntracer).to_environment_id(environment);

    MAIN_TIMER_END_SEGMENT(SET_PROMISE_ENVIRONMENT_RECORDER);

    analysis_driver(dyntracer).promise_environment_set(info, prom);

    MAIN_TIMER_END_SEGMENT(SET_PROMISE_ENVIRONMENT_ANALYSIS);

    tracer_serializer(dyntracer).serialize(
        TraceSerializer::OPCODE_PROMISE_ENVIRONMENT_ASSIGN, info.prom_id,
        environment_id);
}

void gc_unmark(dyntracer_t *dyntracer, const SEXP expression) {
    switch (TYPEOF(expression)) {
        case PROMSXP:
            return gc_promise_unmark(dyntracer, expression);
        case CLOSXP:
            return gc_closure_unmark(dyntracer, expression);
        default:
            return;
    }
}

void gc_promise_unmark(dyntracer_t *dyntracer, const SEXP promise) {
    MAIN_TIMER_RESET();

    prom_addr_t addr = get_sexp_address(promise);
    prom_id_t id = get_promise_id(dyntracer, promise);
    auto &promise_origin = tracer_state(dyntracer).promise_origin;

    MAIN_TIMER_END_SEGMENT(GC_PROMISE_UNMARKED_RECORDER);

    analysis_driver(dyntracer).gc_promise_unmarked(id, promise);

    MAIN_TIMER_END_SEGMENT(GC_PROMISE_UNMARKED_ANALYSIS);

    auto iter = promise_origin.find(id);
    if (iter != promise_origin.end()) {
        // If this is one of our traced promises,
        // delete it from origin map because it is ready to be GCed
        promise_origin.erase(iter);
    }

    tracer_state(dyntracer).promise_ids.erase(addr);

    MAIN_TIMER_END_SEGMENT(GC_PROMISE_UNMARKED_RECORD_KEEPING);
}

void gc_closure_unmark(dyntracer_t *dyntracer, const SEXP function) {
    MAIN_TIMER_RESET();

    remove_function_definition(dyntracer, function);

    MAIN_TIMER_END_SEGMENT(GC_FUNCTION_UNMARKED_RECORD_KEEPING);
}

void gc_entry(dyntracer_t *dyntracer, R_size_t size_needed) {
    MAIN_TIMER_RESET();

    tracer_state(dyntracer).increment_gc_trigger_counter();

    MAIN_TIMER_END_SEGMENT(GC_ENTRY_RECORDER);
}

void gc_exit(dyntracer_t *dyntracer, int gc_counts) {
    MAIN_TIMER_RESET();

    gc_info_t info{tracer_state(dyntracer).get_gc_trigger_counter()};

    MAIN_TIMER_END_SEGMENT(GC_EXIT_RECORDER);

    debug_serializer(dyntracer).serialize_gc_exit(info);
}

void vector_alloc(dyntracer_t *dyntracer, int sexptype, long length, long bytes,
                  const char *srcref) {
    MAIN_TIMER_RESET();

    type_gc_info_t info{tracer_state(dyntracer).get_gc_trigger_counter(),
                        sexptype, length, bytes};

    MAIN_TIMER_END_SEGMENT(VECTOR_ALLOC_RECORDER);

    debug_serializer(dyntracer).serialize_vector_alloc(info);

    analysis_driver(dyntracer).vector_alloc(info);

    MAIN_TIMER_END_SEGMENT(VECTOR_ALLOC_ANALYSIS);
}

void new_environment(dyntracer_t *dyntracer, const SEXP rho) {
    MAIN_TIMER_RESET();

    // fn_id_t fn_id = (tracer_state(dyntracer).fun_stack.back().function_id);
    stack_event_t event = get_last_on_stack_by_type(
        tracer_state(dyntracer).full_stack, stack_type::CALL);
    fn_id_t fn_id = event.type == stack_type::NONE
                        ? compute_hash("")
                        : event.function_info.function_id;

    env_id_t env_id = tracer_state(dyntracer).environment_id_counter++;

    MAIN_TIMER_END_SEGMENT(NEW_ENVIRONMENT_RECORDER);

    debug_serializer(dyntracer).serialize_new_environment(env_id, fn_id);
    tracer_state(dyntracer).environments[rho] =
        std::pair<env_id_t, unordered_map<string, var_id_t>>(env_id, {});

    tracer_serializer(dyntracer).serialize(
        TraceSerializer::OPCODE_ENVIRONMENT_CREATE, env_id);

    MAIN_TIMER_END_SEGMENT(NEW_ENVIRONMENT_WRITE_TRACE);
}

void context_entry(dyntracer_t *dyntracer, const RCNTXT *cptr) {
    MAIN_TIMER_RESET();

    stack_event_t event;
    event.context_id = (rid_t)cptr;
    event.type = stack_type::CONTEXT;
    tracer_state(dyntracer).full_stack.push_back(event);
    debug_serializer(dyntracer).serialize_begin_ctxt(cptr);

    MAIN_TIMER_END_SEGMENT(CONTEXT_ENTRY_STACK);
}

void context_jump(dyntracer_t *dyntracer, const RCNTXT *cptr,
                  const SEXP return_value, int restart) {
    MAIN_TIMER_RESET();

    unwind_info_t info;
    info.jump_context = ((rid_t)cptr);
    info.restart = restart;

    adjust_stacks(dyntracer, info);

    MAIN_TIMER_END_SEGMENT(CONTEXT_JUMP_STACK);

    analysis_driver(dyntracer).context_jump(info);

    MAIN_TIMER_END_SEGMENT(CONTEXT_JUMP_ANALYSIS);

    debug_serializer(dyntracer).serialize_unwind(info);
}

void context_exit(dyntracer_t *dyntracer, const RCNTXT *cptr) {
    MAIN_TIMER_RESET();

    stack_event_t event = tracer_state(dyntracer).full_stack.back();
    if (event.type == stack_type::CONTEXT && ((rid_t)cptr) == event.context_id)
        tracer_state(dyntracer).full_stack.pop_back();
    else
        dyntrace_log_warning("Context trying to remove context %d from full "
                             "stack, but %d is on top of stack.",
                             ((rid_t)cptr), event.context_id);
    debug_serializer(dyntracer).serialize_end_ctxt(cptr);

    MAIN_TIMER_END_SEGMENT(CONTEXT_EXIT_STACK);
}

void adjust_stacks(dyntracer_t *dyntracer, unwind_info_t &info) {
    while (!tracer_state(dyntracer).full_stack.empty()) {
        stack_event_t element = (tracer_state(dyntracer).full_stack.back());

        // if (info.jump_target == element.enclosing_environment)
        //    break;
        if (element.type == stack_type::CONTEXT)
            if (info.jump_context == element.context_id)
                break;
            else
                info.unwound_frames.push_back(element);
        else if (element.type == stack_type::CALL) {
            tracer_serializer(dyntracer).serialize(
                TraceSerializer::OPCODE_FUNCTION_CONTEXT_JUMP, element.call_id);
            info.unwound_frames.push_back(element);
        } else if (element.type == stack_type::PROMISE) {
            tracer_serializer(dyntracer).serialize(
                TraceSerializer::OPCODE_PROMISE_CONTEXT_JUMP,
                element.promise_id);
            info.unwound_frames.push_back(element);
        } else /* if (element.type == stack_type::NONE) */
            dyntrace_log_error("NONE object found on tracer's full stack.");

        tracer_state(dyntracer).full_stack.pop_back();
    }
}

void environment_action(dyntracer_t *dyntracer, const SEXP symbol, SEXP value,
                        const SEXP rho, const std::string &action) {
    bool exists = true;
    prom_id_t promise_id = tracer_state(dyntracer).enclosing_promise_id();
    env_id_t environment_id = tracer_state(dyntracer).to_environment_id(rho);
    var_id_t variable_id =
        tracer_state(dyntracer).to_variable_id(symbol, rho, exists);
    // std::string value = serialize_sexp(expression);
    // std::string exphash = compute_hash(value.c_str());
    // serialize variable iff it has not been seen before.
    // if it has been seen before, then it has already been serialized.

    MAIN_TIMER_END_SEGMENT(ENVIRONMENT_ACTION_RECORDER);

    if (!exists) {
        debug_serializer(dyntracer).serialize_variable(
            variable_id, CHAR(PRINTNAME(symbol)), environment_id);
    }

    debug_serializer(dyntracer).serialize_variable_action(promise_id,
                                                          variable_id, action);

    std::string action_id = action + " " + std::to_string(variable_id);
    debug_serializer(dyntracer).serialize_interference_information(action_id);

    if (action == TraceSerializer::OPCODE_ENVIRONMENT_REMOVE) {
        tracer_serializer(dyntracer).serialize(
            action, tracer_state(dyntracer).to_environment_id(rho), variable_id,
            CHAR(PRINTNAME(symbol)));
    } else {
        tracer_serializer(dyntracer).serialize(
            action, tracer_state(dyntracer).to_environment_id(rho), variable_id,
            CHAR(PRINTNAME(symbol)), value_type_to_string(value));
    }

    MAIN_TIMER_END_SEGMENT(ENVIRONMENT_ACTION_WRITE_TRACE);
}

void environment_variable_define(dyntracer_t *dyntracer, const SEXP symbol,
                                 const SEXP value, const SEXP rho) {
    MAIN_TIMER_RESET();

    analysis_driver(dyntracer).environment_define_var(symbol, value, rho);

    MAIN_TIMER_END_SEGMENT(ENVIRONMENT_ACTION_ANALYSIS);

    environment_action(dyntracer, symbol, value, rho,
                       TraceSerializer::OPCODE_ENVIRONMENT_DEFINE);
}

void environment_variable_assign(dyntracer_t *dyntracer, const SEXP symbol,
                                 const SEXP value, const SEXP rho) {
    MAIN_TIMER_RESET();

    analysis_driver(dyntracer).environment_assign_var(symbol, value, rho);

    MAIN_TIMER_END_SEGMENT(ENVIRONMENT_ACTION_ANALYSIS);

    environment_action(dyntracer, symbol, value, rho,
                       TraceSerializer::OPCODE_ENVIRONMENT_ASSIGN);
}

void environment_variable_remove(dyntracer_t *dyntracer, const SEXP symbol,
                                 const SEXP rho) {
    MAIN_TIMER_RESET();

    analysis_driver(dyntracer).environment_remove_var(symbol, rho);

    MAIN_TIMER_END_SEGMENT(ENVIRONMENT_ACTION_ANALYSIS);

    environment_action(dyntracer, symbol, R_UnboundValue, rho,
                       TraceSerializer::OPCODE_ENVIRONMENT_REMOVE);
}

void environment_variable_lookup(dyntracer_t *dyntracer, const SEXP symbol,
                                 const SEXP value, const SEXP rho) {
    MAIN_TIMER_RESET();

    analysis_driver(dyntracer).environment_lookup_var(symbol, value, rho);

    MAIN_TIMER_END_SEGMENT(ENVIRONMENT_ACTION_ANALYSIS);

    environment_action(dyntracer, symbol, value, rho,
                       TraceSerializer::OPCODE_ENVIRONMENT_LOOKUP);
}
