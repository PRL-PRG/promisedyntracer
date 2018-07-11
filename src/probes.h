#ifndef __PROBES_H__
#define __PROBES_H__

#include "Context.h"
#include "recorder.h"
#include "utilities.h"

#define R_USE_SIGNALS 1
#include "Defn.h"

extern "C" {

void begin(dyntracer_t *dyntracer, const SEXP prom);
void end(dyntracer_t *dyntracer);
void function_entry(dyntracer_t *dyntracer, const SEXP call, const SEXP op,
                    const SEXP rho);
void function_exit(dyntracer_t *dyntracer, const SEXP call, const SEXP op,
                   const SEXP rho, const SEXP retval);
void print_entry_info(dyntracer_t *dyntracer, const SEXP call,
                      const SEXP op, const SEXP rho, function_type fn_type);
void builtin_entry(dyntracer_t *dyntracer, const SEXP call, const SEXP op,
                   const SEXP rho);
void specialsxp_entry(dyntracer_t *dyntracer, const SEXP call,
                      const SEXP op, const SEXP rho);
void print_exit_info(dyntracer_t *dyntracer, const SEXP call,
                     const SEXP op, const SEXP rho, function_type fn_type,
                     const SEXP retval);
void builtin_exit(dyntracer_t *dyntracer, const SEXP call, const SEXP op,
                  const SEXP rho, const SEXP retval);
void specialsxp_exit(dyntracer_t *dyntracer, const SEXP call,
                     const SEXP op, const SEXP rho, const SEXP retval);
void promise_created(dyntracer_t *dyntracer, const SEXP prom,
                     const SEXP rho);
void promise_force_entry(dyntracer_t *dyntracer, const SEXP promise);
void promise_force_exit(dyntracer_t *dyntracer, const SEXP promise);
void promise_value_lookup(dyntracer_t *dyntracer, const SEXP promise, int in_force);
void promise_expression_lookup(dyntracer_t *dyntracer, const SEXP promise, int in_force);
void promise_environment_lookup(dyntracer_t *dyntracer, const SEXP promise, int in_force);
void promise_value_set(dyntracer_t *dyntracer, const SEXP promise, int in_force);
void promise_expression_set(dyntracer_t *dyntracer, const SEXP promise, int in_force);
void promise_environment_set(dyntracer_t *dyntracer, const SEXP promise, int in_force);
void gc_promise_unmarked(dyntracer_t *dyntracer, const SEXP promise);
void gc_function_unmarked(dyntracer_t *dyntracer, const SEXP function);
void gc_entry(dyntracer_t *dyntracer, R_size_t size_needed);
void gc_exit(dyntracer_t *dyntracer, int gc_count, double vcells,
             double ncells);
void vector_alloc(dyntracer_t *dyntracer, int sexptype, long length,
                  long bytes, const char *srcref);
void new_environment(dyntracer_t *dyntracer, const SEXP rho);
void begin_ctxt(dyntracer_t *dyntracer, const RCNTXT *);
void jump_ctxt(dyntracer_t *dyntracer,  const RCNTXT *, SEXP return_value, int restart);
void end_ctxt(dyntracer_t *dyntracer, const RCNTXT *);
void environment_define_var(dyntracer_t *dyntracer, const SEXP symbol,
                            const SEXP value, const SEXP rho);
void environment_assign_var(dyntracer_t *dyntracer, const SEXP symbol,
                            const SEXP value, const SEXP rho);
void environment_remove_var(dyntracer_t *dyntracer, const SEXP symbol,
                            const SEXP rho);
void environment_lookup_var(dyntracer_t *dyntracer, const SEXP symbol,
                            const SEXP value, const SEXP rho);
}
#endif /* __PROBES_H__ */
