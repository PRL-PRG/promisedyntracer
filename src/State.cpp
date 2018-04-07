#include "State.h"
#include "utilities.h"

void tracer_state_t::start_pass(dyntrace_context_t *context, const SEXP prom) {
    prom_id_t prom_id = make_promise_id(context, prom);
    promise_origin[prom_id] = 0;
}

void tracer_state_t::finish_pass() { promise_origin.clear(); }

tracer_state_t::tracer_state_t() {
    clock_id = 0;
    call_id_counter = 0;
    fn_id_counter = 0;
    prom_id_counter = 0;
    prom_neg_id_counter = 0;
    argument_id_sequence = 0;
    gc_trigger_counter = 0;
    closure_counter = 0;
    special_counter = 0;
    builtin_counter = 0;
    environment_id_counter = 0;
    variable_id_counter = 0;
    toplevel_ena = 0;
    toplevel_end = 0;
    toplevel_enr = 0;
    toplevel_enl = 0;
    promise_ena = 0;
    promise_end = 0;
    promise_enr = 0;
    promise_enl = 0;
    function_ena = 0;
    function_end = 0;
    function_enr = 0;
    function_enl = 0;
}

void tracer_state_t::increment_closure_counter() { closure_counter++; }

void tracer_state_t::increment_special_counter() { special_counter++; }

void tracer_state_t::increment_builtin_counter() { builtin_counter++; }

void tracer_state_t::increment_gc_trigger_counter() { gc_trigger_counter++; }

int tracer_state_t::get_closure_counter() const { return closure_counter; }

int tracer_state_t::get_special_counter() const { return special_counter; }

int tracer_state_t::get_builtin_counter() const { return builtin_counter; }

int tracer_state_t::get_gc_trigger_counter() const {
    return gc_trigger_counter;
}

int tracer_state_t::get_closure_calls() {
    int calls = closure_counter;
    closure_counter = 0;
    return calls;
}

int tracer_state_t::get_special_calls() {
    int calls = special_counter;
    special_counter = 0;
    return calls;
}

int tracer_state_t::get_builtin_calls() {
    int calls = builtin_counter;
    builtin_counter = 0;
    return calls;
}

env_id_t tracer_state_t::to_environment_id(SEXP rho) {
    const auto &iter = environments.find(rho);
    if (iter == environments.end()) {
        env_id_t environment_id = environment_id_counter++;
        environments[rho] =
            std::pair<env_id_t, unordered_map<string, var_id_t>>(environment_id,
                                                                 {});
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
stack_event_t get_last_on_stack_by_type(vector<stack_event_t> &stack,
                                        stack_type type) {
    for (vector<stack_event_t>::reverse_iterator i = stack.rbegin();
         i != stack.rend(); ++i)
        if (type == i->type)
            return *i;

    return make_dummy_stack_event();
}

stack_event_t get_from_back_of_stack_by_type(vector<stack_event_t> &stack,
                                             stack_type type, int rposition) {
    int rindex = rposition;
    for (vector<stack_event_t>::reverse_iterator i = stack.rbegin();
         i != stack.rend(); ++i)
        if (type == i->type)
            if (rindex == 0)
                return *i;
            else
                rindex--;

    return make_dummy_stack_event();
}

void tracer_state_t::create_promise_environment_action(prom_id_t promise_id) {
    promise_environment_action[promise_id] = {0, 0, 0, 0, 0, 0, 0, 0};
}

void tracer_state_t::update_promise_environment_action(prom_id_t promise_id,
                                                       std::string action,
                                                       bool transitive) {
    if (promise_environment_action.find(promise_id) ==
        promise_environment_action.end())
        promise_environment_action[promise_id] = {0, 0, 0, 0, 0, 0, 0, 0};

    int index = transitive ? 4 : 0;
    if (action == OPCODE_ENVIRONMENT_DEFINE)
        index += 0;
    else if (action == OPCODE_ENVIRONMENT_ASSIGN)
        index += 1;
    else if (action == OPCODE_ENVIRONMENT_REMOVE)
        index += 2;
    else if (action == OPCODE_ENVIRONMENT_LOOKUP)
        index += 3;

    promise_environment_action[promise_id][index] += 1;

    if (!transitive) {
        if (action == OPCODE_ENVIRONMENT_DEFINE)
            promise_end += 1;
        else if (action == OPCODE_ENVIRONMENT_ASSIGN)
            promise_ena += 1;
        else if (action == OPCODE_ENVIRONMENT_REMOVE)
            promise_enr += 1;
        else if (action == OPCODE_ENVIRONMENT_LOOKUP)
            promise_enl += 1;
    }
}

std::vector<int>
tracer_state_t::remove_promise_environment_action(prom_id_t promise_id) {
    auto actions = promise_environment_action.find(promise_id);
    if (actions != promise_environment_action.end()) {
        auto result = std::move(actions->second);
        promise_environment_action.erase(actions);
        return result;
    } else {
        return {0, 0, 0, 0, 0, 0, 0, 0};
    }
}

// void tracer_state_t::create_function_environment_action(fn_id_t function_id)
// {
//     if (function_environment_action.find(function_id) ==
//         function_environment_action.end())
//         function_environment_action[function_id] = {0, 0, 0, 0, 0, 0, 0, 0};
// }

void tracer_state_t::update_function_environment_action(fn_id_t function_id,
                                                        std::string action,
                                                        bool transitive) {
    if (function_environment_action.find(function_id) ==
        function_environment_action.end())
        function_environment_action[function_id] = {0, 0, 0, 0, 0, 0, 0, 0};

    int index = transitive ? 4 : 0;
    if (action == OPCODE_ENVIRONMENT_DEFINE)
        index += 0;
    else if (action == OPCODE_ENVIRONMENT_ASSIGN)
        index += 1;
    else if (action == OPCODE_ENVIRONMENT_REMOVE)
        index += 2;
    else if (action == OPCODE_ENVIRONMENT_LOOKUP)
        index += 3;

    function_environment_action[function_id][index] += 1;

    if (!transitive) {
        if (action == OPCODE_ENVIRONMENT_DEFINE)
            function_end += 1;
        else if (action == OPCODE_ENVIRONMENT_ASSIGN)
            function_ena += 1;
        else if (action == OPCODE_ENVIRONMENT_REMOVE)
            function_enr += 1;
        else if (action == OPCODE_ENVIRONMENT_LOOKUP)
            function_enl += 1;
    }
}

std::vector<int>
tracer_state_t::remove_function_environment_action(fn_id_t function_id) {
    auto actions = function_environment_action.find(function_id);
    if (actions != function_environment_action.end()) {
        auto result = std::move(actions->second);
        function_environment_action.erase(actions);
        return result;
    } else {
        return {0, 0, 0, 0, 0, 0, 0, 0};
    }
}

void tracer_state_t::update_toplevel_action(std::string action) {
    if (action == OPCODE_ENVIRONMENT_DEFINE)
        toplevel_end += 1;
    else if (action == OPCODE_ENVIRONMENT_ASSIGN)
        toplevel_ena += 1;
    else if (action == OPCODE_ENVIRONMENT_REMOVE)
        toplevel_enr += 1;
    else if (action == OPCODE_ENVIRONMENT_LOOKUP)
        toplevel_enl += 1;
}
