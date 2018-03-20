#include "DebugSerializer.h"
#include <string>
#include <sstream>
#include "Context.h"

using namespace std;

DebugSerializer::DebugSerializer(int verbose)
        : indentation(0),
          verbose(verbose),
          state(nullptr),
          has_state(false) {}

string DebugSerializer::log_line(const stack_event_t &event) {
    stringstream line;
    line << "{type=";
    switch (event.type) {
        case stack_type::PROMISE: line << "promise id=" << event.promise_id; break;
        case stack_type::CALL: line << "call id=" << event.call_id; break;
        case stack_type::CONTEXT: line << "context id=" << event.context_id; break;
        case stack_type ::NONE: line << "none"; break;
    }
    line // << " env=" << event.enclosing_environment
         << "}";
    return line.str();
}

string DebugSerializer::log_line(const function_type &type) {
    switch (type) {
        case function_type::CLOSURE: return "closure";
        case function_type::BUILTIN: return "built-in";
        case function_type::SPECIAL: return "special";
        case function_type::TRUE_BUILTIN: return "true built-in";
    }
    return nullptr;
}

string DebugSerializer::log_line(const arglist_t &arguments) {
    stringstream line;
    line << "[";
    auto all = arguments.all();
    for (auto i = all.begin(); i != all.end(); ++i) {
        if (i != all.begin())
            line << " ";

        const arg_t argument = ((*i).get());
        line << "{name=" << get<0>(argument)
             << " arg_id=" << get<1>(argument)
             << " prom_id=" << get<2>(argument)
             << "}";
    }
    line << "]";
    return line.str();
}

string DebugSerializer::log_line(const sexp_type &type) {
    switch (type) {
        case sexp_type::ACTIVE_BINDING: return "active_binding";
        case sexp_type::ANY: return "ANY";
        case sexp_type::BCODE: return "BCODE";
        case sexp_type::BUILTIN: return "BUILTIN";
        case sexp_type::CHAR: return "CHAR";
        case sexp_type::CLOS: return "CLOS";
        case sexp_type::CPLX: return "CPLX";
        case sexp_type::DOT: return "DOT";
        case sexp_type::ENV: return "ENV";
        case sexp_type::EXPR: return "EXPR";
        case sexp_type::EXTPTR: return "EXTPTR";
        case sexp_type::INT: return "INT";
        case sexp_type::LANG: return "LANG";
        case sexp_type::LGL: return "LGL";
        case sexp_type::LIST: return "LIST";
        case sexp_type::NIL: return "NIL";
        case sexp_type::OMEGA: return "...";
        case sexp_type::PROM: return "PROM";
        case sexp_type::RAW: return "RAW";
        case sexp_type::REAL: return "REAL";
        case sexp_type::S4: return "S4";
        case sexp_type::SPECIAL: return "SPECIAL";
        case sexp_type::STR: return "STR";
        case sexp_type::SYM: return "SYM";
        case sexp_type::VEC: return "VEC";
        case sexp_type::WEAKREF: return "WEAKREF";
    }
    return nullptr;
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
    line << log_line(info.fn_type)
         << " name=" << info.name
         << " fn_id=" << info.fn_id
         << " call_id=" << info.call_id
         << " env_ptr=" << info.call_ptr
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
    line << log_line(info.fn_type)
         << " name=" << info.name
         << " fn_id=" << info.fn_id
         << " call_id=" << info.call_id
         << " env_ptr=" << info.call_ptr
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
         << " effective_distance_from_origin=" << info.effective_distance_from_origin;
         // skipped: lifestyle
    return line.str();
}

string DebugSerializer::log_line(const RCNTXT *cptr) {
    stringstream line;
    line << "context " << ((rid_t) cptr); // << " cloenv="<< ((rid_t) cptr->cloenv);
    return line.str();
}

string DebugSerializer::log_line(const unwind_info_t &info) {
    stringstream line;
    line << "unwind"
         << " target=" << info.jump_context
         << " unwound_promises=[";
    for (auto i = info.unwound_promises.begin(); i != info.unwound_promises.end(); ++i) {
        if (i != info.unwound_promises.begin())
            line << " ";
        line << (*i);
    }
    line << "] unwound_calls=[";
    for (auto i = info.unwound_calls.begin(); i != info.unwound_calls.end(); ++i) {
        if (i != info.unwound_calls.begin())
            line << " ";
        line << (*i);
    }
    line << "] unwound_contexts=[";
    for (auto i = info.unwound_contexts.begin(); i != info.unwound_contexts.end(); ++i) {
        if (i != info.unwound_contexts.begin())
            line << " ";
        line << (*i);
    }
    line << "]";
    return line.str();
}

string DebugSerializer::log_line(const gc_info_t &info) {
    stringstream line;
    line << "gc"
         << " counter=" << info.counter
         << " ncells=" << info.ncells
         << " vcells=" << info.vcells;
    return line.str();
}

string DebugSerializer::log_line(const prom_gc_info_t &info) {
    stringstream line;
    line << "prom_gc"
         << " promise_id=" << info.promise_id
         << " event=" << info.event
         << " gc_trigger_counter=" << info.gc_trigger_counter;
    return line.str();
}

string DebugSerializer::log_line(const type_gc_info_t &info) {
    stringstream line;
    line << "type_gc"
         << " type_id=" << info.type
         << " length=" << info.length
         << " bytes=" << info.bytes
         << " gc_trigger_counter=" << info.gc_trigger_counter;
    return line.str();
}

void DebugSerializer::indent() {
    indentation++;
}

string DebugSerializer::unindent() {
    stringstream line;
//    if (indentation > 10)
//        line << "|[" << (indentation / 10) << "]  ";
//    for (int i = 1; i < indentation%10; ++i)
//        line << "│  ";
//    line << "┴";
    indentation--;
    return line.str();
}

string DebugSerializer::print_stack() {
    if (!has_state) {
        return "*";
    }

    stringstream line;

//    line << " \n    FUN=[[";
    //int i = 0;
   // int fun_threshold = state->fun_stack.size() - 10 - 1;
    //fun_threshold = fun_threshold < 0 ? 0 : fun_threshold;
    //if (state->fun_stack.size() > 10)
        //line << "..";
//    for (auto fun : state->fun_stack) {
//        //if (i > fun_threshold)
//            line << "("
//                 << " F" << fun.call_id
//                 << " cloenv="
//                 << fun.enclosing_environment
//                 << ")";
//        //i++;
//    }
//    line << " ]] (" << state->fun_stack.size() << ")";

    line << " \n   [[";
    for (auto event : state->full_stack) {
        //if (i > full_threshold)
            line << " "
                 << (event.type == stack_type::PROMISE ? "P" :
                     (event.type == stack_type::CALL ? "F" : "C"))
                 << event.call_id;
                 //<< " cloenv="
                 //<< event.enclosing_environment
                 //<< ")";
    }
    line << " ]] (" << state->full_stack.size() << ")";
   return line.str();
}

string DebugSerializer::prefix() {
    return "";
    stringstream line;
    if (indentation > 10)
        line << "|[" << (indentation / 10) << "]  ";
    for (int i = 1; i < indentation%10; ++i)
        line << "│  ";
    if (indentation > 0)
        line << "├─ ";
    return line.str();
}

void DebugSerializer::serialize_promise_lifecycle(const prom_gc_info_t &info) {
    if (!(verbose > 1)) return;
    cerr << prefix() << log_line(info) << print_stack() << endl;
}

void DebugSerializer::serialize_gc_exit(const gc_info_t &info) {
    if (!(verbose > 1)) return;
    cerr << prefix() << log_line(info) << print_stack() << endl;
}

void DebugSerializer::serialize_vector_alloc(const type_gc_info_t &info) {
    if (!(verbose > 1)) return;
    cerr << prefix() << log_line(info) << print_stack() << endl;
}

void DebugSerializer::serialize_promise_expression_lookup(const prom_info_t &info) {
    if (!(verbose > 0)) return;
    cerr << prefix() << log_line(info) << print_stack() << endl;
}

void DebugSerializer::serialize_promise_lookup(const prom_info_t &info) {
    if (!(verbose > 0)) return;
    cerr << prefix() << log_line(info) << print_stack() << endl;
}

void DebugSerializer::serialize_function_entry(const closure_info_t &info) {
    if (!(verbose > 0)) return;
    cerr << prefix() << ">>> " << log_line(info) << print_stack() << endl;
    indent();
}

void DebugSerializer::serialize_function_exit(const closure_info_t &info) {
    if (!(verbose > 0)) return;
    cerr << unindent() << "<<< " << log_line(info) << print_stack() << endl;
}

void DebugSerializer::serialize_builtin_entry(const builtin_info_t &info) {
    if (!(verbose > 0)) return;
    cerr << prefix() << ">>> " << log_line(info) << print_stack() << endl;
    indent();
}

void DebugSerializer::serialize_builtin_exit(const builtin_info_t &info) {
    if (!(verbose > 0)) return;
    cerr << unindent() << "<<< " << log_line(info) << print_stack() << endl;
}

void DebugSerializer::serialize_force_promise_entry(const prom_info_t &info) {
    if (!(verbose > 0)) return;
    cerr << prefix()
         << ">>> force " << log_line(info)
         << print_stack()
         << endl;
    indent();
}

void DebugSerializer::serialize_force_promise_exit(const prom_info_t &info) {
    if (!(verbose > 0)) return;
    cerr << unindent()
         << "<<< force "
         << log_line(info)
         << print_stack()
         << endl;
}

void DebugSerializer::serialize_promise_created(const prom_basic_info_t &info) {
    if (!(verbose > 0)) return;
    cerr << prefix()
         << "=== create " << log_line(info)
         << print_stack()
         << endl;
}

void DebugSerializer::serialize_promise_argument_type(const prom_id_t prom_id,
                                                      bool default_argument) {
    if (!(verbose > 1)) return;
    cerr << prefix()
         << "prom_arg_type prom_id=" << prom_id
         << " default_argument=" << default_argument
         << print_stack()
         << endl;
}

void DebugSerializer::serialize_new_environment(const env_id_t env_id,
                                              const fn_id_t fun_id) {
    if (!(verbose > 1)) return;
    cerr << prefix()
         << "new_environment env_id=" << env_id
         << " fun_id=" << fun_id
         << print_stack()
         << endl;
}

void DebugSerializer::serialize_begin_ctxt(const RCNTXT * cptr) {
    if (!(verbose > 0)) return;
    indent();
    cerr << prefix() << ">>> " << log_line(cptr) << print_stack() << endl;
}

void DebugSerializer::serialize_unwind(const unwind_info_t &info) {
    if (!(verbose > 0)) return;
    cerr << prefix() << "=== " << log_line(info) << print_stack() << endl;
    size_t unwindings = info.unwound_calls.size()
                        + info.unwound_promises.size();
    //for (size_t i = 1; i <= unwindings; ++i) {
    //    cerr << unindent() << endl;
    //}
}

void DebugSerializer::serialize_end_ctxt(const RCNTXT * cptr) {
    if (!(verbose > 0)) return;
    unindent();
    cerr << prefix() << "<<< " << log_line(cptr) << print_stack() << endl;
}

void DebugSerializer::serialize_variable(var_id_t variable_id,
                                         const std::string &name,
                                         env_id_t environment_id) {
    if (!(verbose > 1)) return;
    cerr << prefix()
         << "variable var_id=" << variable_id
         << " name=" << name
         << " env_id=" << environment_id
         << print_stack()
         << endl;
}

void DebugSerializer::serialize_variable_action(prom_id_t promise_id,
                                              var_id_t variable_id,
                                              const std::string &action) {
    if (!(verbose > 1)) return;
    cerr << prefix()
         << "variable action=" << action
         << " var_id=" << variable_id
         << " prom_id=" << promise_id
         << print_stack()
         << endl;
}

void DebugSerializer::serialize_interference_information(
        const std::string &info) {
    if (!(verbose > 1)) return;
    cerr << prefix()
         << "interference " << print_stack() << info
         << endl;
}

void DebugSerializer::serialize_start_trace() {
    if (!(verbose > 0)) return;
    indent();
    cerr << prefix() << "begin" << print_stack() << endl;
    indent();
}

void DebugSerializer::serialize_metadatum(const string &key, const string &value) {}

void DebugSerializer::serialize_finish_trace() {
    if (!(verbose > 0)) return;
    unindent();
    cerr << prefix() << "end" << print_stack() << endl;
    unindent();
}

void DebugSerializer::setState(tracer_state_t * state) {
    this->state = state;
    this->has_state = true;
}

bool DebugSerializer::needsState() {
    return !(this->has_state);
}
