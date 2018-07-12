#ifndef __PROBES_H__
#define __PROBES_H__

#include "Context.h"
#include "recorder.h"
#include "utilities.h"

#define R_USE_SIGNALS 1
#include "Defn.h"

extern "C" {

void dyntrace_entry(dyntracer_t *dyntracer, SEXP expression, SEXP environment);
void dyntrace_exit(dyntracer_t *dyntracer, SEXP expression, SEXP environment,
                   SEXP result, int error);
void closure_entry(dyntracer_t *dyntracer, const SEXP call, const SEXP op,
                   const SEXP args, const SEXP rho);
void closure_exit(dyntracer_t *dyntracer, const SEXP call, const SEXP op,
                  const SEXP args, const SEXP rho, const SEXP retval);
void print_entry_info(dyntracer_t *dyntracer, const SEXP call, const SEXP op,
                      const SEXP rho, function_type fn_type);
void builtin_entry(dyntracer_t *dyntracer, const SEXP call, const SEXP op,
                   const SEXP args, const SEXP rho);
void special_entry(dyntracer_t *dyntracer, const SEXP call, const SEXP op,
                   const SEXP args, const SEXP rho);
void print_exit_info(dyntracer_t *dyntracer, const SEXP call, const SEXP op,
                     const SEXP rho, function_type fn_type, const SEXP retval);
void builtin_exit(dyntracer_t *dyntracer, const SEXP call, const SEXP op,
                  const SEXP args, const SEXP rho, const SEXP retval);
void special_exit(dyntracer_t *dyntracer, const SEXP call, const SEXP op,
                  const SEXP args, const SEXP rho, const SEXP retval);
void promise_created(dyntracer_t *dyntracer, const SEXP prom, const SEXP rho);
void promise_force_entry(dyntracer_t *dyntracer, const SEXP promise);
void promise_force_exit(dyntracer_t *dyntracer, const SEXP promise);
void promise_value_lookup(dyntracer_t *dyntracer, const SEXP promise);
void promise_expression_lookup(dyntracer_t *dyntracer, const SEXP promise);
void promise_environment_lookup(dyntracer_t *dyntracer, const SEXP promise);
void promise_value_assign(dyntracer_t *dyntracer, const SEXP promise,
                          const SEXP value);
void promise_expression_assign(dyntracer_t *dyntracer, const SEXP promise,
                               const SEXP expression);
void promise_environment_assign(dyntracer_t *dyntracer, const SEXP promise,
                                const SEXP environment);
void gc_unmark(dyntracer_t *dyntracer, const SEXP expression);
void gc_promise_unmark(dyntracer_t *dyntracer, const SEXP promise);
void gc_closure_unmark(dyntracer_t *dyntracer, const SEXP closure);
void gc_function_unmarked(dyntracer_t *dyntracer, const SEXP function);
void gc_entry(dyntracer_t *dyntracer, R_size_t size_needed);
void gc_exit(dyntracer_t *dyntracer, int gc_count, double vcells,
             double ncells);
void vector_alloc(dyntracer_t *dyntracer, int sexptype, long length, long bytes,
                  const char *srcref);
void new_environment(dyntracer_t *dyntracer, const SEXP rho);
void context_entry(dyntracer_t *dyntracer, const RCNTXT *);
void context_jump(dyntracer_t *dyntracer, const RCNTXT *, SEXP return_value,
                  int restart);
void context_exit(dyntracer_t *dyntracer, const RCNTXT *);
void environment_variable_define(dyntracer_t *dyntracer, const SEXP symbol,
                                 const SEXP value, const SEXP rho);
void environment_variable_assign(dyntracer_t *dyntracer, const SEXP symbol,
                                 const SEXP value, const SEXP rho);
void environment_variable_remove(dyntracer_t *dyntracer, const SEXP symbol,
                                 const SEXP rho);
void environment_variable_lookup(dyntracer_t *dyntracer, const SEXP symbol,
                                 const SEXP value, const SEXP rho);
}
#endif /* __PROBES_H__ */
