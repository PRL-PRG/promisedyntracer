#include "State.h"
#include "utilities.h"

void tracer_state_t::start_pass(dyntrace_context_t *context, const SEXP prom) {
    // We have to make sure the stack is not empty
    // when referring to the promise created by call to Rdt.
    // This is just a dummy call and environment.
    //    call_stack_elem_t e;
    //    e.call_id = (call_id_t) 0;
    //    e.function_id = compute_hash("");
    //    e.type = function_type::CLOSURE;
    //    e.enclosing_environment = 0; //(env_addr_t) R_GlobalContext->cloenv;
    //    fun_stack.push_back(e);
    //    context_t ctx;
    //    ctx.context = 0;
    //    ctx.environment = 0;
    //    curr_env_stack.push_back(ctx);

    //prom_addr_t prom_addr = get_sexp_address(prom);
    prom_id_t prom_id = make_promise_id(context, prom);
    promise_origin[prom_id] = 0;
}

void tracer_state_t::finish_pass() {
    //    fun_stack.pop_back();
    //    curr_env_stack.pop_back();

    promise_origin.clear();
}

//void tracer_state_t::adjust_stacks_noninclusive(SEXP rho, unwind_info_t &info) {
//    env_addr_t call_addr;
//
//    while (!full_stack.empty() && (call_addr = curr_env_stack.back().environment)) {
//        stack_event_t event_from_fullstack = full_stack.back();
//        if (event_from_fullstack.type == stack_type::CALL) {
//            // Make sure stacks are in synch.
//            auto call_from_funstack = (fun_stack.back());
//            if (call_from_funstack.call_id != event_from_fullstack.call_id)
//                dyntrace_log_error("Disagreement between tracer stack: %s",
//                                "top of function stack != top of full stack");
//
//            if (get_sexp_address(rho) == call_addr)
//                break;                              // We are already there. Just quit, don't touch the stacks.
//
//            cout << "                 call_addr: " << call_addr << endl;
//
//            cout << " popping from curr_end_stack: " << curr_env_stack.back() << endl;
//            curr_env_stack.pop_back();
//
//            if (!curr_env_stack.empty()) {            // Peek the next element in the stack, stop if we're there.
//                cout << "    curr_env_stack.top(-1): " << curr_env_stack.back() << endl;
//                cout << "     get_sexp_address(rho): " << get_sexp_address(rho) << endl;
//                cout << "                      COND: " << (get_sexp_address(rho) == curr_env_stack.back()) << endl;
//                if (get_sexp_address(rho) == curr_env_stack.back()) {
//                    curr_env_stack.push_back(call_addr); // Re-push the top of the stack
//                    break;                          // Leave the top of each stack in place and quit.
//                }
//            }
//
//            // Remove call from all three stacks.
//            cout << "    popping from full_stack: " << full_stack.back().call_id << endl;
//            full_stack.pop_back();
//            cout << "     popping from fun_stack: " << fun_stack.back().call_id << endl;
//            fun_stack.pop_back();
//
//            // Register that a call was unwound in the info object.
//            info.unwound_calls.push_back(event_from_fullstack.call_id);
//        } else if (event_from_fullstack.type == stack_type::PROMISE) {
//            if (!curr_env_stack.empty())            // Peek the next element in the stack, stop if we're there.
//                if (get_sexp_address(rho) == curr_env_stack.back()) {
//                    curr_env_stack.push_back(call_addr); // Re-push the top of the stack
//                    break;                          // Leave the top of each stack in place and quit.
//                }
//
//            // Remove only from full stack, because there are no promises on either fun_stack or curr_env_stack.
//            full_stack.pop_back();
//
//            // Register that a promise was unwound in the info object.
//            info.unwound_promises.push_back(event_from_fullstack.promise_id);
//        } else /* if (event_from_fullstack.type == stack_type::NONE) */ {
//            dyntrace_log_error("NONE object found on tracer's full stack.");
//        }
//    }
//}

void tracer_state_t::adjust_stacks(unwind_info_t &info) {
    while (!full_stack.empty()) {
        stack_event_t event_from_fullstack = (full_stack.back());

        //if (info.jump_target == event_from_fullstack.enclosing_environment)
        //    break;
        if (event_from_fullstack.type == stack_type::CONTEXT)
            if (info.jump_context == event_from_fullstack.context_id)
                break;
            else
                info.unwound_contexts.push_back(event_from_fullstack.context_id);
        else if (event_from_fullstack.type == stack_type::CALL)
            info.unwound_calls.push_back(event_from_fullstack.call_id);
        else if (event_from_fullstack.type == stack_type::PROMISE)
            info.unwound_promises.push_back(event_from_fullstack.promise_id);
        else /* if (event_from_fullstack.type == stack_type::NONE) */
            dyntrace_log_error("NONE object found on tracer's full stack.");

        full_stack.pop_back();
    }

//    vector<rid_t> unwound_contexts;
//    while (!curr_env_stack.empty()) {
//        context_t ctx = curr_env_stack.back();
//
//        if (info.jump_context == ctx.context)
//            break;
//
//        unwound_contexts.push_back(ctx.context);
//        info.unwound_environments.push_back(ctx.environment);
//        curr_env_stack.pop_back();
//    }
//
//    vector<call_id_t> unwound_calls_from_fun_stack;
//    while (!fun_stack.empty()) {
//        call_stack_elem_t call_from_fun_stack = (fun_stack.back());
//
//        if (info.jump_target == call_from_fun_stack.enclosing_environment)
//            break;
//
//        unwound_calls_from_fun_stack.push_back(call_from_fun_stack.call_id);
//        fun_stack.pop_back();
//    }

    // Check if the stacks were in synch:
//    if (info.unwound_calls.size() != unwound_calls_from_fun_stack.size())
//        dyntrace_log_error("Fun stack and full stack each unwound different number of calls.");
//    for (auto fun_i = unwound_calls_from_fun_stack.begin(), full_i = info.unwound_calls.begin();
//         fun_i != unwound_calls_from_fun_stack.end() || full_i != info.unwound_calls.end(); ++fun_i, ++full_i) {
//        if ((*fun_i) != (*full_i))
//            dyntrace_log_error("Fun stack and full stack unwound different calls (%l, %l)", *fun_i, *full_i);
//    }
}

//void tracer_state_t::adjust_stacks(SEXP rho, unwind_info_t &info) {
//    env_addr_t call_addr;
//
//    while (!full_stack.empty() && (call_addr = curr_env_stack.top())) {
//        stack_event_t event_from_fullstack = full_stack.back();
//        if (event_from_fullstack.type == stack_type::CALL) {
//            // Make sure stacks are in synch.
//            auto call_from_funstack = (fun_stack.back());
//            if (get<0>(call_from_funstack) != event_from_fullstack.call_id)
//                dyntrace_log_error("Disagreement between tracer stack: %s",
//                                   "top of function stack != top of full stack");
//
//            if (get_sexp_address(rho) == call_addr)
//                break;
//
//            // Remove call from all three stacks.
//            full_stack.pop_back();
//            fun_stack.pop_back();
//            curr_env_stack.pop();
//
//            // Register that a call was unwound in the info object.
//            info.unwound_calls.push_back(event_from_fullstack.call_id);
//        } else if (event_from_fullstack.type == stack_type::PROMISE) {
//            // Remove only from full stack, because there are no promises on either fun_stack or curr_env_stack.
//            full_stack.pop_back();
//
//            // Register that a promise was unwound in the info object.
//            info.unwound_promises.push_back(event_from_fullstack.promise_id);
//        } else /* if (event_from_fullstack.type == stack_type::NONE) */ {
//            dyntrace_log_error("NONE object found on tracer's full stack.");
//        }
//    }
//}

// XXX remnant of RDT_CALL_ID
//(call_id = fun_stack.top()) && get_sexp_address(rho) != call_id
/*
    while (!fun_stack.empty() && (call_addr = curr_env_stack.top()) &&
           get_sexp_address(rho) != call_addr) {
        //auto elem = (fun_stack.back());
        //call_id = get<0>(elem);
        curr_env_stack.pop();
        fun_stack.pop_back();
    }

    while (!full_stack.empty() && (call_addr = curr_env_stack.top()) &&
            get_sexp_address(rho) != call_addr) {

        auto event = full_stack.back();
        if (event.type == stack_type::CALL) {
            if (event.call_id == call_id)
                goto outside_promise_loop;
        } else {
            info.unwound_promises.push_back(event.promise_id);
        }

        auto elem = (full_stack.back());
        call_id = get<0>(elem);
        curr_env_stack.pop();
        fun_stack.pop_back();


        info.unwound_calls.push_back(call_id);


            auto event = full_stack.back();
            full_stack.pop_back();

            if (event.type == stack_type::CALL) {
                if (event.call_id == call_id)
                    goto outside_promise_loop;
            } else {
                info.unwound_promises.push_back(event.promise_id);
            }
        }
    outside_promise_loop:;
    }
}*/

// void tracer_state_t::adjust_prom_stack(SEXP rho, vector<prom_id_t> &
// unwound_promises) {
//    prom_id_t prom_id;
//    env_addr_t call_addr;
//
//    // XXX remnant of RDT_CALL_ID
//    //(call_id = fun_stack.top()) && get_sexp_address(rho) != call_id
//
//    while (!fun_stack.empty() && (call_addr = curr_env_stack.top()) &&
//    get_sexp_address(rho) != call_addr) {
//        prom_stack_elem_t elem = prom_stack.back();
//        prom_id = elem.first;
//        curr_env_stack.pop();
//
//        fun_stack.pop_back();
//
//        unwound_promises.push_back(prom_id);
//    }
//}

tracer_state_t::tracer_state_t() {
    clock_id = 0;
    call_id_counter = 0;
    fn_id_counter = 0;
    prom_id_counter = 0;
    prom_neg_id_counter = 0;
    argument_id_sequence = 0;
    gc_trigger_counter = 0;
    environment_id_counter = 0;
    variable_id_counter = 0;
}

env_id_t tracer_state_t::to_environment_id(SEXP rho) {
    const auto &iter = environments.find(rho);
    if (iter == environments.end()) {
        env_id_t environment_id = environment_id_counter++;
        environments[rho] =
            std::pair<env_id_t, unordered_map<string, var_id_t>>(environment_id, {});
        return environment_id;
    } else {
        return (iter->second).first;
    }
}

var_id_t tracer_state_t::to_variable_id(SEXP symbol, SEXP rho, bool &exists) {
    var_id_t variable_id;
    const auto &iter = environments.find(rho);
    assert(iter != environments.end());
    auto &variables = (iter->second).second;
    const auto &iter2 = variables.find(CHAR(PRINTNAME(symbol)));
    if (iter2 == variables.end()) {
        exists = false;
        variable_id = variable_id_counter++;
        variables[std::string(CHAR(PRINTNAME(symbol)))] = variable_id;
    } else {
        exists = true;
        variable_id = iter2->second;
    }
    return variable_id;
}

// This function returns -1 if we are not in an enclosing promise scope.
// -1 is also used for foreign promises, so the client should appropriately
// process this information downstream.
// TODO use a more general function to get stuff form stack by type
prom_id_t tracer_state_t::enclosing_promise_id() {

    for (std::vector<stack_event_t>::reverse_iterator iterator =
             full_stack.rbegin();
         iterator != full_stack.rend(); ++iterator) {
        if (iterator->type == stack_type::PROMISE)
            return iterator->promise_id;
    }
    return -1;
}

static stack_event_t make_dummy_stack_event() {
    stack_event_t dummy_event;
    dummy_event.type = stack_type::NONE;
    dummy_event.enclosing_environment = 0;
    dummy_event.context_id = 0;
    return dummy_event;
}

// TODO return pointer?
stack_event_t get_last_on_stack_by_type(vector<stack_event_t> &stack, stack_type type) {
    for (vector<stack_event_t>::reverse_iterator i = stack.rbegin(); i != stack.rend(); ++i)
        if (type == i->type)
            return *i;

    return make_dummy_stack_event();
}

stack_event_t get_from_back_of_stack_by_type(vector<stack_event_t> &stack, stack_type type, int rposition) {
    int rindex = rposition;
    for (vector<stack_event_t>::reverse_iterator i = stack.rbegin(); i != stack.rend(); ++i)
        if (type == i->type)
            if (rindex == 0)
                return *i;
            else
                rindex--;

    return make_dummy_stack_event();
}

