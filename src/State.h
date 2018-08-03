#ifndef __STATE_H__
#define __STATE_H__

#include "sexptypes.h"
#include "stdlibs.h"

using std::string;
using std::pair;
using std::tuple;
using std::map;
using std::hash;
using std::get;
using std::vector;
using std::unordered_map;
using std::unordered_set;

#define RID_INVALID 0 //(rid_t) - 1

const int RDT_SQL_LOOKUP_PROMISE = 0x0;
const int RDT_SQL_LOOKUP_PROMISE_EXPRESSION = 0x1;
const int RDT_SQL_FORCE_PROMISE = 0xF;

// Typical human-readable representation
typedef uintptr_t rid_t; // hexadecimal
typedef intptr_t rsid_t; // hexadecimal

typedef rid_t prom_addr_t; // hexadecimal
typedef rid_t env_addr_t;  // hexadecimal
// typedef rsid_t prom_id_t;  // hexadecimal
typedef rsid_t prom_id_t;
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

struct arg_t {
    arg_id_t id;
    string name;
    sexptype_t value_type;
    sexptype_t name_type;
    prom_id_t promise_id; // only set if sexptype_t == PROM
    SEXP promise_environment;
    bool default_argument;
    int formal_parameter_position;
};

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
typedef prom_addr_t prom_key_t;

struct call_info_t {
    function_type fn_type;
    fn_id_t fn_id;
    SEXP fn_addr; // TODO unnecessary?
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

    stack_event_t parent_on_stack;
    sexptype_t return_value_type;
    string call_expression;
    int formal_parameter_count;
};

typedef vector<arg_t> arglist_t;

struct closure_info_t : call_info_t {
    arglist_t arguments;
};

struct builtin_info_t : call_info_t {};

// FIXME would it make sense to add type of action here?
struct prom_basic_info_t {
    prom_id_t prom_id;

    sexptype_t prom_type;
    full_sexp_type full_type;

    prom_id_t in_prom_id;
    stack_event_t parent_on_stack;
    int depth;
    std::string expression;
};

struct prom_info_t : prom_basic_info_t {
    call_id_t in_call_id;
    call_id_t from_call_id;
    sexptype_t return_type;
};

struct unwind_info_t {
    rid_t jump_context;
    int restart;
    vector<stack_event_t> unwound_frames;
};

struct gc_info_t {
    int counter;
};

struct type_gc_info_t {
    int gc_trigger_counter;
    int type;
    long length;
    long bytes;
};

prom_id_t get_promise_id(dyntracer_t *dyntracer, SEXP promise);
prom_id_t make_promise_id(dyntracer_t *dyntracer, SEXP promise,
                          bool negative = false);
call_id_t make_funcall_id(dyntracer_t *dyntracer, SEXP);
string get_function_definition(dyntracer_t *dyntracer, const SEXP function);
void remove_function_definition(dyntracer_t *dyntracer, const SEXP function);
fn_id_t get_function_id(dyntracer_t *dyntracer, const string &def,
                        bool builtin = false);
fn_addr_t get_function_addr(SEXP func);

// Returns false if function already existed, true if it was registered now
bool register_inserted_function(dyntracer_t *dyntracer, fn_id_t id);

bool function_already_inserted(fn_id_t id);
bool negative_promise_already_inserted(dyntracer_t *dyntracer, prom_id_t id);
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

stack_event_t get_last_on_stack_by_type(vector<stack_event_t> &stack,
                                        stack_type type);
stack_event_t get_from_back_of_stack_by_type(vector<stack_event_t> &stack,
                                             stack_type type, int rposition);

prom_id_t get_parent_promise(dyntracer_t *dyntracer);
arg_id_t get_argument_id(dyntracer_t *dyntracer, call_id_t call_id,
                         const string &argument);

void update_closure_arguments(closure_info_t &info, dyntracer_t *dyntracer,
                              const call_id_t call_id, const SEXP formals,
                              const SEXP args, const SEXP environment);

size_t get_no_of_ancestor_promises_on_stack(dyntracer_t *dyntracer);
size_t get_no_of_ancestors_on_stack();
size_t get_no_of_ancestor_calls_on_stack();

string recursive_type_to_string(recursion_type);

struct tracer_state_t {
    vector<stack_event_t> full_stack; // Should be reset on each tracer pass

    // Map from promise IDs to call IDs
    unordered_map<prom_id_t, call_id_t>
        promise_origin; // Should be reset on each tracer pass
    unordered_set<prom_id_t> fresh_promises;
    // Map from promise address to promise ID;
    unordered_map<prom_addr_t, prom_id_t> promise_ids;
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

    unordered_map<SEXP, string> function_definitions;

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

    std::unordered_map<
        SEXP, std::pair<env_id_t, std::unordered_map<std::string, var_id_t>>>
        environments;

    void finish_pass();
    env_id_t to_environment_id(SEXP rho);
    var_id_t to_variable_id(SEXP symbol, SEXP rho, bool &exists);
    var_id_t to_variable_id(const std::string &symbol, SEXP rho, bool &exists);
    prom_id_t enclosing_promise_id();
    void remove_environment(const SEXP rho);
    void increment_gc_trigger_counter();

    int get_gc_trigger_counter() const;

    tracer_state_t();
};

#endif /* __STATE_H__ */
