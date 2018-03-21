#ifndef __STATE_H__
#define __STATE_H__

#include "sexptypes.h"
#include "stdlibs.h"

using namespace std;

#define RID_INVALID (rid_t) - 1

const int RDT_SQL_LOOKUP_PROMISE = 0x0;
const int RDT_SQL_LOOKUP_PROMISE_EXPRESSION = 0x1;
const int RDT_SQL_FORCE_PROMISE = 0xF;

// Typical human-readable representation
typedef uintptr_t rid_t; // hexadecimal
typedef intptr_t rsid_t; // hexadecimal

typedef rid_t prom_addr_t; // hexadecimal
typedef rid_t env_addr_t;  // hexadecimal
typedef rsid_t prom_id_t;  // hexadecimal
typedef rid_t
    call_id_t; // integer TODO this is pedantic, but shouldn't this be int?

typedef string fn_id_t;  // integer
typedef rid_t fn_addr_t; // hexadecimal
typedef string fn_key_t; // pun
typedef int env_id_t;
typedef int var_id_t;
typedef unsigned long int arg_id_t; // integer

typedef int event_t;

typedef pair<call_id_t, string> arg_key_t;

rid_t get_sexp_address(SEXP e);

typedef tuple<string, arg_id_t, prom_id_t, bool, int> arg_t;
typedef tuple<arg_id_t, prom_id_t, bool, int> anon_arg_t;

enum class function_type {
    CLOSURE = 0,
    BUILTIN = 1,
    SPECIAL = 2,
    TRUE_BUILTIN = 3
};

enum class recursion_type {
    UNKNOWN = 0,
    RECURSIVE = 1,
    NOT_RECURSIVE = 2,
    MUTUALLY_RECURSIVE = 3
};

enum class lifestyle_type {
    VIRGIN = 0,
    LOCAL = 1,
    BRANCH_LOCAL = 2,
    ESCAPED = 3,
    IMMEDIATE_LOCAL = 4,
    IMMEDIATE_BRANCH_LOCAL = 5
};

enum class stack_type { PROMISE = 1, CALL = 2, CONTEXT = 3, NONE = 0 };

struct stack_event_t {
    stack_type type;
    union {
        prom_id_t promise_id;
        call_id_t call_id;
        rid_t context_id;
    };
    env_addr_t enclosing_environment;
    // Only initialized for type == CALL
    struct {
        fn_id_t function_id;
        function_type type;
    } function_info;
};

typedef map<std::string, std::string> metadata_t;

struct call_stack_elem_t {
    call_id_t call_id;
    fn_id_t function_id;
    function_type type;
    env_addr_t enclosing_environment;
};

struct context_t {
    rid_t context;
    env_addr_t environment;
};

// typedef pair<prom_id_t, call_id_t> prom_stack_elem_t;
typedef tuple<prom_id_t, unsigned int, unsigned int> prom_key_t;

struct prom_id_triple_hash {
  public:
    size_t operator()(const prom_key_t &p) const {
        auto h1 = hash<prom_id_t>{}(get<0>(p));
        auto h2 = hash<unsigned int>{}(get<1>(p));
        auto h3 = hash<unsigned int>{}(get<2>(p));
        // super simple≥...
        return (h1 << 16) | (h2 << 8) | h3;
    }
};

struct call_info_t {
    function_type fn_type;
    fn_id_t fn_id;
    fn_addr_t fn_addr; // TODO unnecessary?
    string fn_definition;
    string definition_location;
    string callsite_location;
    bool fn_compiled;

    string name; // fully qualified function name, if available
    call_id_t call_id;
    env_addr_t call_ptr;
    call_id_t
        parent_call_id; // the id of the parent call that executed this call
    prom_id_t in_prom_id;

    recursion_type recursion; // TODO unnecessary?

    stack_event_t parent_on_stack;
    sexp_type return_value_type;
};

class arglist_t {
    vector<arg_t> args;
    vector<arg_t> ddd_kw_args;
    vector<arg_t> ddd_pos_args;
    mutable vector<reference_wrapper<const arg_t>> arg_refs;
    mutable bool update_arg_refs;

    template <typename T> void push_back_tmpl(T &&value, bool ddd) {
        if (ddd) {
            arg_t new_value = value;
            string &arg_name = get<0>(new_value);
            arg_name = "...[" + arg_name + "]";
            ddd_kw_args.push_back(new_value);
        } else {
            args.push_back(forward<T>(value));
        }
        update_arg_refs = true;
    }

    template <typename T> void push_back_anon_tmpl(T &&value) {
        string arg_name = "...[" + to_string(ddd_pos_args.size()) + "]";
        // prepend string arg_name to anon_arg_t value
        arg_t new_value = tuple_cat(make_tuple(arg_name), value);

        ddd_pos_args.push_back(new_value);
        update_arg_refs = true;
    }

  public:
    arglist_t() { update_arg_refs = true; }

    void push_back(const arg_t &value, bool ddd = false) {
        push_back_tmpl(value, ddd);
    }

    void push_back(arg_t &&value, bool ddd = false) {
        push_back_tmpl(value, ddd);
    }

    void push_back(const anon_arg_t &value) { push_back_anon_tmpl(value); }

    void push_back(anon_arg_t &&value) { push_back_anon_tmpl(value); }

    // Return vector of references to elements of our three inner vectors
    // so we can iterate over all of them in one for loop.
    vector<reference_wrapper<const arg_t>> all() const {
        if (update_arg_refs) {
            arg_refs.assign(args.begin(), args.end());
            arg_refs.insert(arg_refs.end(), ddd_kw_args.begin(),
                            ddd_kw_args.end());
            arg_refs.insert(arg_refs.end(), ddd_pos_args.begin(),
                            ddd_pos_args.end());
            update_arg_refs = false;
        }

        return arg_refs;
    }

    size_t size() const {
        if (update_arg_refs) {
            all();
        }
        return arg_refs.size();
    }
};

struct closure_info_t : call_info_t {
    arglist_t arguments;
};

struct builtin_info_t : call_info_t {};

// FIXME would it make sense to add type of action here?
struct prom_basic_info_t {
    prom_id_t prom_id;

    sexp_type prom_type;
    full_sexp_type full_type;

    prom_id_t in_prom_id;
    stack_event_t parent_on_stack;
    int depth;
};

struct prom_info_t : prom_basic_info_t {
    string name;
    call_id_t in_call_id;
    call_id_t from_call_id;
    lifestyle_type lifestyle;
    int effective_distance_from_origin;
    int actual_distance_from_origin;
    sexp_type return_type;
};

struct unwind_info_t {
    env_addr_t  jump_target;
    rid_t  jump_context;
    vector<env_addr_t> unwound_environments;
    vector<call_id_t> unwound_calls;
    vector<prom_id_t> unwound_promises;
    vector<rid_t> unwound_contexts;
};

struct gc_info_t {
    int counter;
    double ncells;
    double vcells;
    int builtin_calls;
    int special_calls;
    int closure_calls;
};

struct prom_lifecycle_info_t {
    prom_id_t promise_id;
    event_t event;
    int gc_trigger_counter;
    int builtin_counter;
    int special_counter;
    int closure_counter;
};

struct type_gc_info_t {
    int gc_trigger_counter;
    int type;
    long length;
    long bytes;
};

prom_id_t get_promise_id(dyntrace_context_t *context, SEXP promise);
prom_id_t make_promise_id(dyntrace_context_t *context, SEXP promise,
                          bool negative = false);
call_id_t make_funcall_id(dyntrace_context_t *context, SEXP);
fn_id_t get_function_id(dyntrace_context_t *context, SEXP func,
                        bool builtin = false);
fn_addr_t get_function_addr(SEXP func);

// Returns false if function already existed, true if it was registered now
bool register_inserted_function(dyntrace_context_t *context, fn_id_t id);

bool function_already_inserted(fn_id_t id);
bool negative_promise_already_inserted(dyntrace_context_t *context,
                                       prom_id_t id);
template <typename T>
void get_stack_parent(T &info, vector<stack_event_t> &stack) {
    // put the body here
    static_assert(std::is_base_of<prom_basic_info_t, T>::value ||
                      std::is_base_of<prom_info_t, T>::value ||
                      std::is_base_of<call_info_t, T>::value,
                  "get_stack_parent is only applicable for arguments of types: "
                  "prom_basic_info_t,  prom_info_t, or call_info_t.");

    if (!stack.empty()) {
        stack_event_t stack_elem = stack.back();
        // parent type
        info.parent_on_stack.type = stack_elem.type;
        switch (info.parent_on_stack.type) {
            case stack_type::PROMISE:
                info.parent_on_stack.promise_id = stack_elem.call_id;
                break;
            case stack_type::CALL:
                info.parent_on_stack.call_id = stack_elem.promise_id;
                break;
            case stack_type::NONE:
                break;
        }
    } else {
        info.parent_on_stack.type = stack_type::NONE;
        info.parent_on_stack.call_id = 0;
    }
}

template <typename T>
void get_stack_parent2(T &info, vector<stack_event_t> &stack) {
    // put the body here
    static_assert(std::is_base_of<prom_basic_info_t, T>::value ||
                  std::is_base_of<prom_info_t, T>::value ||
                  std::is_base_of<call_info_t, T>::value,
                  "get_stack_parent is only applicable for arguments of types: "
                          "prom_basic_info_t,  prom_info_t, or call_info_t.");

    if (stack.size() > 1) {
        stack_event_t stack_elem = stack.rbegin()[1];
        info.parent_on_stack.type = stack_elem.type;
        switch (info.parent_on_stack.type) {
            case stack_type::PROMISE:
                info.parent_on_stack.promise_id = stack_elem.call_id;
                break;
            case stack_type::CALL:
                info.parent_on_stack.call_id = stack_elem.promise_id;
                break;
            case stack_type::NONE:
                break;
        }
    } else {
        info.parent_on_stack.type = stack_type::NONE;
        info.parent_on_stack.call_id = 0;
    }
}

stack_event_t get_last_on_stack_by_type(vector<stack_event_t> &stack, stack_type type);
stack_event_t get_from_back_of_stack_by_type(vector<stack_event_t> &stack, stack_type type, int rposition);

prom_id_t get_parent_promise(dyntrace_context_t *context);
arg_id_t get_argument_id(dyntrace_context_t *context, call_id_t call_id,
                         const string &argument);
arglist_t get_arguments(dyntrace_context_t *, call_id_t, SEXP op, SEXP rho);

size_t get_no_of_ancestor_promises_on_stack(dyntrace_context_t *context);
size_t get_no_of_ancestors_on_stack();
size_t get_no_of_ancestor_calls_on_stack();

string recursive_type_to_string(recursion_type);

struct tracer_state_t {
    int clock_id; // Should be kept across Rdt calls (unless overwrite is true)
    vector<stack_event_t> full_stack;    // Should be reset on each tracer pass

    // Map from promise IDs to call IDs
    unordered_map<prom_id_t, call_id_t>
        promise_origin; // Should be reset on each tracer pass
    unordered_set<prom_id_t> fresh_promises;
    // Map from promise address to promise ID;
    unordered_map<prom_key_t, prom_id_t, prom_id_triple_hash> promise_ids;
    unordered_map<prom_id_t, int> promise_lookup_gc_trigger_counter;
    env_id_t environment_id_counter;
    var_id_t variable_id_counter;
    call_id_t call_id_counter; // IDs assigned should be globally unique but we
                               // can reset it after each pass if overwrite is
                               // true)
    prom_id_t fn_id_counter;   // IDs assigned should be globally unique but we
                               // can reset it after each pass if overwrite is
                               // true)
    prom_id_t prom_id_counter; // IDs assigned should be globally unique but we
                               // can reset it after each pass if overwrite is
                               // true)
    prom_id_t prom_neg_id_counter;

    unordered_map<fn_key_t, fn_id_t> function_ids; // Should be kept across Rdt
                                                   // calls (unless overwrite is
                                                   // true)
    unordered_set<fn_id_t> already_inserted_functions; // Should be kept across
                                                       // Rdt calls (unless
                                                       // overwrite is true)
    unordered_set<prom_id_t>
        already_inserted_negative_promises; // Should be kept
                                            // across Rdt
                                            // calls (unless
                                            // overwrite is
                                            // true)
    arg_id_t argument_id_sequence; // Should be globally unique (can reset
                                   // between tracer calls if overwrite is true)
    map<arg_key_t, arg_id_t> argument_ids; // Should be kept across Rdt calls
                                           // (unless overwrite is true)
    int gc_trigger_counter; // Incremented each time there is a gc_entry

    int builtin_counter; // Increment each time a builtin is called
    int special_counter; // Increment each time a special is called
    int closure_counter; // Increment each time a closure is called

    unordered_map<SEXP, std::pair<env_id_t, unordered_map<string, var_id_t>>>
        environments;
    void start_pass(dyntrace_context_t *context, const SEXP prom);
    void finish_pass();
    // When doing longjump (exception thrown, etc.) this function gets the
    // target environment and unwinds function call stack until that
    // environment is on top.
    void adjust_stacks(unwind_info_t &info);
    env_id_t to_environment_id(SEXP rho);
    var_id_t to_variable_id(SEXP symbol, SEXP rho, bool &exists);
    prom_id_t enclosing_promise_id();

    void increment_closure_counter();
    void increment_special_counter();
    void increment_builtin_counter();
    void increment_gc_trigger_counter();

    int get_closure_counter() const;
    int get_special_counter() const;
    int get_builtin_counter() const;
    int get_gc_trigger_counter() const;

    int get_closure_calls();
    int get_special_calls();
    int get_builtin_calls();

    tracer_state_t();
};
#endif /* __STATE_H__ */
