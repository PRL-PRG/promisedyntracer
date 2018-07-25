#include "tracer.h"
#include "probes.h"

extern "C" {

// verbose:
//     0: quiet,
//     1: debug tracer prints calls and promises,
//     2: debug tracer prints everything
//     -1: SQL queries,
SEXP create_dyntracer(SEXP trace_filepath, SEXP truncate, SEXP enable_trace,
                      SEXP verbose, SEXP output_dir, SEXP analysis_switch) {
    void *context = new Context(
        sexp_to_string(trace_filepath), sexp_to_bool(truncate),
        sexp_to_bool(enable_trace), sexp_to_bool(verbose),
        sexp_to_string(output_dir), to_analysis_switch(analysis_switch));

    /* calloc initializes the memory to zero. This ensures that probes not
       attached will be NULL. Replacing calloc with malloc will cause
       segfaults. */
    dyntracer_t *dyntracer = (dyntracer_t *)calloc(1, sizeof(dyntracer_t));
    dyntracer->probe_dyntrace_entry = dyntrace_entry;
    dyntracer->probe_dyntrace_exit = dyntrace_exit;
    dyntracer->probe_closure_entry = closure_entry;
    dyntracer->probe_closure_exit = closure_exit;
    dyntracer->probe_builtin_entry = builtin_entry;
    dyntracer->probe_builtin_exit = builtin_exit;
    dyntracer->probe_special_entry = special_entry;
    dyntracer->probe_special_exit = special_exit;
    dyntracer->probe_gc_unmark = gc_unmark;
    dyntracer->probe_promise_force_entry = promise_force_entry;
    dyntracer->probe_promise_force_exit = promise_force_exit;
    dyntracer->probe_gc_allocate = gc_allocate;
    dyntracer->probe_promise_value_lookup = promise_value_lookup;
    dyntracer->probe_promise_expression_lookup = promise_expression_lookup;
    dyntracer->probe_promise_environment_lookup = promise_environment_lookup;
    dyntracer->probe_promise_value_assign = promise_value_assign;
    dyntracer->probe_promise_expression_assign = promise_expression_assign;
    dyntracer->probe_promise_environment_assign = promise_environment_assign;
    dyntracer->probe_gc_entry = gc_entry;
    dyntracer->probe_gc_exit = gc_exit;
    dyntracer->probe_context_entry = context_entry;
    dyntracer->probe_context_jump = context_jump;
    dyntracer->probe_context_exit = context_exit;
    dyntracer->probe_environment_variable_define = environment_variable_define;
    dyntracer->probe_environment_variable_assign = environment_variable_assign;
    dyntracer->probe_environment_variable_remove = environment_variable_remove;
    dyntracer->probe_environment_variable_lookup = environment_variable_lookup;
    dyntracer->state = context;
    return dyntracer_to_sexp(dyntracer, "dyntracer.promise");
}

static void destroy_promise_dyntracer(dyntracer_t *dyntracer) {
    /* free dyntracer iff it has not already been freed.
       this check ensures that multiple calls to destroy_dyntracer on the same
       object do not crash the process. */
    if (dyntracer) {
        delete (static_cast<Context *>(dyntracer->state));
        free(dyntracer);
    }
}

SEXP destroy_dyntracer(SEXP dyntracer_sexp) {
    return dyntracer_destroy_sexp(dyntracer_sexp, destroy_promise_dyntracer);
}

} // extern "C"
