#include "Context.h"
#include "State.h"
#include "lookup.h"
#include "utilities.h"
#include <cassert>
#include <sstream>

rid_t get_sexp_address(SEXP e) { return (rid_t)e; }

prom_id_t get_promise_id(dyntrace_context_t *context, SEXP promise) {
    if (promise == R_NilValue)
        return RID_INVALID;

    if (TYPEOF(promise) != PROMSXP)
        return RID_INVALID;

    // A new promise is always created for each argument.
    // Even if the argument is already a promise passed from the caller, it gets
    // re-wrapped.
    prom_addr_t prom_addr = get_sexp_address(promise);
    prom_id_t prom_id;

    unsigned int prom_type = TYPEOF(PRCODE(promise));
    unsigned int orig_type =
        (prom_type == BCODESXP) ? TYPEOF(BODY_EXPR(PRCODE(promise))) : 0;
    prom_key_t key(prom_addr, prom_type, orig_type);

    auto &promise_ids = tracer_state(context).promise_ids;
    auto it = promise_ids.find(key);
    if (it != promise_ids.end()) {
        prom_id = it->second;
    } else {
        prom_id = make_promise_id(context, promise, true);
    }

    return prom_id;
}

prom_id_t make_promise_id(dyntrace_context_t *context, SEXP promise,
                          bool negative) {
    if (promise == R_NilValue)
        return RID_INVALID;

    prom_addr_t prom_addr = get_sexp_address(promise);
    prom_id_t prom_id;

    if (negative) {
        prom_id = --tracer_state(context).prom_neg_id_counter;
    } else {
        prom_id = tracer_state(context).prom_id_counter++;
    }
    unsigned int prom_type = TYPEOF(PRCODE(promise));
    unsigned int orig_type =
        (prom_type == 21) ? TYPEOF(BODY_EXPR(PRCODE(promise))) : 0;
    prom_key_t key(prom_addr, prom_type, orig_type);
    tracer_state(context).promise_ids[key] = prom_id;

    auto &already_inserted_negative_promises =
        tracer_state(context).already_inserted_negative_promises;
    already_inserted_negative_promises.insert(prom_id);

    return prom_id;
}

string get_function_definition(dyntrace_context_t *context, const SEXP function) {
    auto &definitions = tracer_state(context).function_definitions;
    auto it = definitions.find(function);
    if (it != definitions.end()) {
#ifdef RDT_DEBUG
        string test = get_expression(function);
        if (it->second.compare(test) != 0) {
            cout << "Function definitions are wrong.";
        }
#endif
        return it->second;
    } else {
        string definition = get_expression(function);
        tracer_state(context).function_definitions[function] = definition;
        return definition;
    }
}

void remove_function_definition(dyntrace_context_t *context, const SEXP function) {
    auto &definitions = tracer_state(context).function_definitions;
    auto it = definitions.find(function);
    if (it != definitions.end())
        tracer_state(context).function_definitions.erase(it);
}

fn_id_t get_function_id(dyntrace_context_t *context,const string &function_definition, bool builtin) {
    fn_key_t definition(function_definition);

    auto &function_ids = tracer_state(context).function_ids;
    auto it = function_ids.find(definition);

    if (it != function_ids.end()) {
        return it->second;
    } else {
        /*Use hash on the function body to compute a unique (hopefully) id
         for each function.*/

        fn_id_t fn_id = compute_hash(definition.c_str());
        tracer_state(context).function_ids[definition] = fn_id;
        return fn_id;
    }
}

bool register_inserted_function(dyntrace_context_t *context, fn_id_t id) {
    auto &already_inserted_functions =
        tracer_state(context).already_inserted_functions;
    auto result = already_inserted_functions.insert(id);
    return result.second;
}

bool negative_promise_already_inserted(dyntrace_context_t *context,
                                       prom_id_t id) {
    auto &already_inserted =
        tracer_state(context).already_inserted_negative_promises;
    return already_inserted.count(id) > 0;
}

bool function_already_inserted(dyntrace_context_t *context, fn_id_t id) {
    auto &already_inserted_functions =
        tracer_state(context).already_inserted_functions;
    return already_inserted_functions.count(id) > 0;
}

fn_addr_t get_function_addr(SEXP func) { return get_sexp_address(func); }

call_id_t make_funcall_id(dyntrace_context_t *context, SEXP function) {
    if (function == R_NilValue)
        return RID_INVALID;

    return ++tracer_state(context).call_id_counter;
}

// FIXME use general parent function by type instead.
prom_id_t get_parent_promise(dyntrace_context_t *context) {
    for (std::vector<stack_event_t>::reverse_iterator iterator =
             tracer_state(context).full_stack.rbegin();
         iterator != tracer_state(context).full_stack.rend(); ++iterator) {
        if (iterator->type == stack_type::PROMISE)
            return iterator->promise_id;
    }
    return 0; // FIXME should return a special value or something
}

size_t get_no_of_ancestor_promises_on_stack(dyntrace_context_t *context) {
    size_t result = 0;
    vector<stack_event_t> & stack = tracer_state(context).full_stack;
    for (auto it = stack.begin(); it != stack.end(); ++it) {
        if (it->type == stack_type::PROMISE)
            result++;
    }
    return result;
}

arg_id_t get_argument_id(dyntrace_context_t *context, call_id_t call_id,
                         const string &argument) {
    arg_id_t argument_id = ++tracer_state(context).argument_id_sequence;
    return argument_id;
}

string recursive_type_to_string(recursion_type type) {
    switch (type) {
        case recursion_type::MUTUALLY_RECURSIVE:
            return "mutually_recursive";
        case recursion_type::RECURSIVE:
            return "recursive";
        case recursion_type::NOT_RECURSIVE:
            return "not_recursive";
        case recursion_type::UNKNOWN:
            return "unknown";
        default:
            return "???????";
    }
}
