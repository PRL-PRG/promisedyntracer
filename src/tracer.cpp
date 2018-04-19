#include "tracer.h"
#include "SqlSerializer.h"
#include "probes.h"
//#include "recorder.h"
//#include <cstring>
//#include <fstream>
//#include <functional>
//#include <sstream>
//#include <stack>
//#include <string>
//#include <unordered_map>
//#include <unordered_set>
//#include <vector>

extern "C" {

// verbose:
//     0: quiet,
//     1: debug tracer prints calls and promises,
//     2: debug tracer prints everything
//     -1: SQL queries,
SEXP create_dyntracer(SEXP database, SEXP schema, SEXP truncate, SEXP verbose) {
    void *context =
        new Context(sexp_to_string(":memory:"), sexp_to_string(schema),
                    sexp_to_bool(truncate), sexp_to_int(verbose));
    /* calloc initializes the memory to zero. This ensures that probes not
       attached will be NULL. Replacing calloc with malloc will cause
       segfaults. */
    dyntracer_t *dyntracer = (dyntracer_t *)calloc(1, sizeof(dyntracer_t));
    dyntracer->probe_begin = begin;
    dyntracer->probe_end = end;
    dyntracer->probe_function_entry = function_entry;
    dyntracer->probe_function_exit = function_exit;
    dyntracer->probe_builtin_entry = builtin_entry;
    dyntracer->probe_builtin_exit = builtin_exit;
    dyntracer->probe_specialsxp_entry = specialsxp_entry;
    dyntracer->probe_specialsxp_exit = specialsxp_exit;
    dyntracer->probe_gc_promise_unmarked = gc_promise_unmarked;
    dyntracer->probe_promise_force_entry = promise_force_entry;
    dyntracer->probe_promise_force_exit = promise_force_exit;
    dyntracer->probe_promise_created = promise_created;
    dyntracer->probe_promise_value_lookup = promise_value_lookup;
    dyntracer->probe_promise_expression_lookup = promise_expression_lookup;
    dyntracer->probe_promise_environment_lookup = promise_environment_lookup;
    dyntracer->probe_promise_value_set = promise_value_set;
    dyntracer->probe_promise_expression_set = promise_expression_set;
    dyntracer->probe_promise_environment_set = promise_environment_set;
    dyntracer->probe_vector_alloc = vector_alloc;
    dyntracer->probe_gc_entry = gc_entry;
    dyntracer->probe_gc_exit = gc_exit;
    dyntracer->probe_new_environment = new_environment;
    dyntracer->probe_begin_ctxt = begin_ctxt;
    dyntracer->probe_jump_ctxt = jump_ctxt;
    dyntracer->probe_end_ctxt = end_ctxt;
    dyntracer->probe_environment_define_var = environment_define_var;
    dyntracer->probe_environment_assign_var = environment_assign_var;
    dyntracer->probe_environment_remove_var = environment_remove_var;
    dyntracer->probe_environment_lookup_var = environment_lookup_var;
    dyntracer->context = context;
    return dyntracer_to_sexp(dyntracer, "dyntracer.promise");
}

static void destroy_promise_dyntracer(dyntracer_t *dyntracer) {
    /* free dyntracer iff it has not already been freed.
       this check ensures that multiple calls to destroy_dyntracer on the same
       object do not crash the process. */
    if (dyntracer) {
        delete (static_cast<Context *>(dyntracer->context));
        free(dyntracer);
    }
}

SEXP destroy_dyntracer(SEXP dyntracer_sexp) {
    return dyntracer_destroy_sexp(dyntracer_sexp, destroy_promise_dyntracer);
}

} // extern "C"
