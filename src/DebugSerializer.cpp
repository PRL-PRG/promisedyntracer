#include "DebugSerializer.h"
#include "Context.h"
#include <sstream>
#include <string>

using namespace std;

DebugSerializer::DebugSerializer(bool verbose)
    : indentation(0), verbose(verbose), state(nullptr), has_state(false) {}

string DebugSerializer::log_line(const stack_event_t &event) {
    stringstream line;
    line << "{type=";
    switch (event.type) {
        case stack_type::PROMISE:
            line << "promise id=" << event.promise_id;
            break;
        case stack_type::CALL:
            line << "call id=" << event.call_id;
            break;
        case stack_type::CONTEXT:
            line << "context id=" << event.context_id;
            break;
        case stack_type::NONE:
            line << "none";
            break;
    }
    line // << " env=" << event.enclosing_environment
        << "}";
    return line.str();
}

string DebugSerializer::log_line(const function_type &type) {
    switch (type) {
        case function_type::CLOSURE:
            return "closure";
        case function_type::BUILTIN:
            return "built-in";
        case function_type::SPECIAL:
            return "special";
        case function_type::TRUE_BUILTIN:
            return "true built-in";
    }
    return nullptr;
}

string DebugSerializer::log_line(const arglist_t &arguments) {
    stringstream line;
    line << "[";
    for (auto i = arguments.begin(); i != arguments.end(); ++i) {
        if (i != arguments.begin())
            line << " ";

        const arg_t &argument = *i;
        line << "{name=" << argument.name << " id=" << argument.id
             << " type=" << log_line(argument.argument_type) << "/"
             << log_line(argument.expression_type)
             << " default=" << argument.default_argument
             << " prom_id=" << argument.promise_id
             << " position=" << argument.formal_parameter_position << "}";
    }
    line << "]";
    return line.str();
}

string DebugSerializer::log_line(const sexptype_t &type) {
    return sexptype_to_string(type);
}

string DebugSerializer::log_line(const full_sexp_type &type) {
    stringstream line;
    for (auto i = type.begin(); i != type.end(); ++i) {
        if (i != type.begin())
            line << "->";
        line << log_line(*i);
    }
    return line.str();
}

string DebugSerializer::log_line(const builtin_info_t &info) {
    stringstream line;
    line << log_line(info.fn_type) << " name=" << info.name
         << " fn_id=" << info.fn_id << " call_id=" << info.call_id
         //<< " env_ptr=" << info.call_ptr
         << " parent=" << log_line(info.parent_on_stack)
         << " parent_call_id=" << info.parent_call_id
         << " parent_prom_id=" << info.in_prom_id
         << " definition_location=" << info.definition_location
         << " callsite_location=" << info.callsite_location
         << " compiled=" << info.fn_compiled
         << " definition=" << info.fn_definition;
    return line.str();
}

string DebugSerializer::log_line(const closure_info_t &info) {
    stringstream line;
    line << log_line(info.fn_type) << " name=" << info.name
         << " fn_id=" << info.fn_id << " call_id=" << info.call_id
         //<< " env_ptr=" << info.call_ptr
         << " args=" << log_line(info.arguments)
         << " parent=" << log_line(info.parent_on_stack)
         << " parent_call_id=" << info.parent_call_id
         << " parent_prom_id=" << info.in_prom_id
         << " definition_location=" << info.definition_location
         << " callsite_location=" << info.callsite_location
         << " compiled=" << info.fn_compiled
         << " definition=" << info.fn_definition;
    return line.str();
}

string DebugSerializer::log_line(const prom_basic_info_t &info) {
    stringstream line;
    line << "promise_basic"
         << " prom_id=" << info.prom_id
         << " prom_type=" << log_line(info.prom_type)
         << " full_type=" << log_line(info.full_type)
         << " in_prom_id=" << info.in_prom_id
         << " parent_on_stack=" << log_line(info.parent_on_stack)
         << " depth=" << info.depth;
    return line.str();
}

string DebugSerializer::log_line(const prom_info_t &info) {
    stringstream line;
    line << "promise"
         // << " name=" << info.name // TODO is this ever set?
         << " prom_id=" << info.prom_id
         << " prom_type=" << log_line(info.prom_type)
         << " return_type=" << log_line(info.return_type)
         << " full_type=" << log_line(info.full_type)
         << " from_call_id=" << info.from_call_id
         << " in_call_id=" << info.in_call_id
         << " in_prom_id=" << info.in_prom_id
         << " parent_on_stack=" << log_line(info.parent_on_stack)
         << " depth=" << info.depth
         << " actual_distance_from_origin=" << info.actual_distance_from_origin
         << " effective_distance_from_origin="
         << info.effective_distance_from_origin;
    // skipped: lifestyle
    return line.str();
}

string DebugSerializer::log_line(const RCNTXT *cptr) {
    stringstream line;
    line << "context " << ((rid_t)cptr);
    // << " cloenv="<< ((rid_t) cptr->cloenv);
    return line.str();
}

string DebugSerializer::log_line(const unwind_info_t &info) {
    stringstream line;
    line << "unwind"
         << " target=" << info.jump_context << " unwound_promises=[";
    // for (auto i = info.unwound_frames.begin();
    //      i != info.unwound_promises.end(); ++i) {
    //     if (i != info.unwound_promises.begin())
    //         line << " ";
    //     line << (*i);
    // }
    // line << "] unwound_calls=[";
    // for (auto i = info.unwound_calls.begin(); i != info.unwound_calls.end();
    //      ++i) {
    //     if (i != info.unwound_calls.begin())
    //         line << " ";
    //     line << (*i);
    // }
    // line << "] unwound_contexts=[";
    // for (auto i = info.unwound_contexts.begin();
    //      i != info.unwound_contexts.end(); ++i) {
    //     if (i != info.unwound_contexts.begin())
    //         line << " ";
    //     line << (*i);
    // }
    // line << "]";
    return line.str();
}

string DebugSerializer::log_line(const gc_info_t &info) {
    stringstream line;
    line << "gc"
         << " counter=" << info.counter << " ncells=" << info.ncells
         << " vcells=" << info.vcells;
    return line.str();
}

string DebugSerializer::log_line(const type_gc_info_t &info) {
    stringstream line;
    line << "type_gc"
         << " type_id=" << info.type << " length=" << info.length
         << " bytes=" << info.bytes
         << " gc_trigger_counter=" << info.gc_trigger_counter;
    return line.str();
}

void DebugSerializer::indent() { indentation++; }

string DebugSerializer::unindent() {
    stringstream line;
    // TODO make optional
    //    if (indentation > 10)
    //        line << "|[" << (indentation / 10) << "]  ";
    //    for (int i = 1; i < indentation%10; ++i)
    //        line << "│  ";
    //    line << "┴";
    indentation--;
    return line.str();
}

string DebugSerializer::print_stack() {
    if (!has_state)
        return "*";

    stringstream line;
    line << " \n   [[";
    for (auto event : state->full_stack) {
        line << " " << (event.type == stack_type::PROMISE
                            ? "P"
                            : (event.type == stack_type::CALL ? "F" : "C"))
             << event.call_id;
    }
    line << " ]] (" << state->full_stack.size() << ")";
    return line.str();
}

string DebugSerializer::prefix() {
    return "";
    // TODO make optional
    //    stringstream line;
    //    if (indentation > 10)
    //        line << "|[" << (indentation / 10) << "]  ";
    //    for (int i = 1; i < indentation%10; ++i)
    //        line << "│  ";
    //    if (indentation > 0)
    //        line << "├─ ";
    //    return line.str();
}

void DebugSerializer::serialize_gc_exit(const gc_info_t &info) {
    if (!verbose)
        return;
    cerr << prefix() << log_line(info) << print_stack() << endl;
}

void DebugSerializer::serialize_vector_alloc(const type_gc_info_t &info) {
    if (!verbose)
        return;
    cerr << prefix() << log_line(info) << print_stack() << endl;
}

void DebugSerializer::serialize_promise_expression_lookup(
    const prom_info_t &info) {
    if (!verbose)
        return;
    cerr << prefix() << log_line(info) << print_stack() << endl;
}

void DebugSerializer::serialize_promise_lookup(const prom_info_t &info) {
    if (!verbose)
        return;
    cerr << prefix() << log_line(info) << print_stack() << endl;
}

void DebugSerializer::serialize_function_entry(const closure_info_t &info) {
    if (!verbose)
        return;
    cerr << prefix() << ">>> " << log_line(info) << print_stack() << endl;
    indent();
}

void DebugSerializer::serialize_function_exit(const closure_info_t &info) {
    if (!verbose)
        return;
    cerr << unindent() << "<<< " << log_line(info) << print_stack() << endl;
}

void DebugSerializer::serialize_builtin_entry(const builtin_info_t &info) {
    if (!verbose)
        return;
    cerr << prefix() << ">>> " << log_line(info) << print_stack() << endl;
    indent();
}

void DebugSerializer::serialize_builtin_exit(const builtin_info_t &info) {
    if (!verbose)
        return;
    cerr << unindent() << "<<< " << log_line(info) << print_stack() << endl;
}

void DebugSerializer::serialize_force_promise_entry(const prom_info_t &info) {
    if (!verbose)
        return;
    cerr << prefix() << ">>> force " << log_line(info) << print_stack() << endl;
    indent();
}

void DebugSerializer::serialize_force_promise_exit(const prom_info_t &info) {
    if (!verbose)
        return;
    cerr << unindent() << "<<< force " << log_line(info) << print_stack()
         << endl;
}

void DebugSerializer::serialize_promise_created(const prom_basic_info_t &info) {
    if (!verbose)
        return;
    cerr << prefix() << "=== create " << log_line(info) << print_stack()
         << endl;
}

void DebugSerializer::serialize_promise_argument_type(const prom_id_t prom_id,
                                                      bool default_argument) {
    if (!verbose)
        return;
    cerr << prefix() << "prom_arg_type prom_id=" << prom_id
         << " default_argument=" << default_argument << print_stack() << endl;
}

void DebugSerializer::serialize_new_environment(const env_id_t env_id,
                                                const fn_id_t fun_id) {
    if (!verbose)
        return;
    cerr << prefix() << "new_environment env_id=" << env_id
         << " fun_id=" << fun_id << print_stack() << endl;
}

void DebugSerializer::serialize_begin_ctxt(const RCNTXT *cptr) {
    if (!verbose)
        return;
    indent();
    cerr << prefix() << ">>> " << log_line(cptr) << print_stack() << endl;
}

void DebugSerializer::serialize_unwind(const unwind_info_t &info) {
    if (!verbose)
        return;
    cerr << prefix() << "=== " << log_line(info) << print_stack() << endl;
    // size_t unwindings = info.unwound_calls.size() +
    // info.unwound_promises.size() + info.unwound_contexts;
    // for (size_t i = 1; i <= unwindings; ++i) {
    //    cerr << unindent() << endl;
    //}
}

void DebugSerializer::serialize_end_ctxt(const RCNTXT *cptr) {
    if (!verbose)
        return;
    unindent();
    cerr << prefix() << "<<< " << log_line(cptr) << print_stack() << endl;
}

void DebugSerializer::serialize_variable(var_id_t variable_id,
                                         const std::string &name,
                                         env_id_t environment_id) {
    if (!verbose)
        return;
    cerr << prefix() << "variable var_id=" << variable_id << " name=" << name
         << " env_id=" << environment_id << print_stack() << endl;
}

void DebugSerializer::serialize_variable_action(prom_id_t promise_id,
                                                var_id_t variable_id,
                                                const std::string &action) {
    if (!verbose)
        return;
    cerr << prefix() << "variable action=" << action
         << " var_id=" << variable_id << " prom_id=" << promise_id
         << print_stack() << endl;
}

void DebugSerializer::serialize_interference_information(
    const std::string &info) {
    if (!verbose)
        return;
    cerr << prefix() << "interference " << print_stack() << info << endl;
}

void DebugSerializer::serialize_start_trace() {
    if (!verbose)
        return;
    indent();
    cerr << prefix() << "begin" << print_stack() << endl;
    indent();
}

void DebugSerializer::serialize_metadatum(const string &key,
                                          const string &value) {}

void DebugSerializer::serialize_finish_trace() {
    if (!verbose)
        return;
    unindent();
    cerr << prefix() << "end" << print_stack() << endl;
    unindent();
}

void DebugSerializer::setState(tracer_state_t *state) {
    this->state = state;
    this->has_state = true;
}

bool DebugSerializer::needsState() { return !(this->has_state); }
