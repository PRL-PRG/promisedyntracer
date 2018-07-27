#include "State.h"
#include "TraceSerializer.h"
#include "utilities.h"

void tracer_state_t::finish_pass() { promise_origin.clear(); }

tracer_state_t::tracer_state_t() {
    call_id_counter = 0;
    fn_id_counter = 0;
    prom_id_counter = 0;
    prom_neg_id_counter = 0;
    argument_id_sequence = 0;
    gc_trigger_counter = 0;
    environment_id_counter = 0;
    variable_id_counter = 0;
}

void tracer_state_t::increment_gc_trigger_counter() { gc_trigger_counter++; }

int tracer_state_t::get_gc_trigger_counter() const {
    return gc_trigger_counter;
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
    return to_variable_id(CHAR(PRINTNAME(symbol)), rho, exists);
}

var_id_t tracer_state_t::to_variable_id(const std::string &symbol, SEXP rho,
                                        bool &exists) {
    var_id_t variable_id;
    const auto &iter = environments.find(rho);
    assert(iter != environments.end());
    auto &variables = (iter->second).second;
    const auto &iter2 = variables.find(symbol);
    if (iter2 == variables.end()) {
        exists = false;
        variable_id = variable_id_counter++;
        variables[symbol] = variable_id;
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
        if (type == i->type) {
            if (rindex == 0)
                return *i;
            else
                rindex--;
        }

    return make_dummy_stack_event();
}
