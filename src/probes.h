#ifndef __PROBES_H__
#define __PROBES_H__

#include "Context.h"
#include "recorder.h"
#include "utilities.h"

extern "C" {

void begin(dyntrace_context_t *context, const SEXP prom);
void end(dyntrace_context_t *context);
void function_entry(dyntrace_context_t *context, const SEXP call, const SEXP op,
                    const SEXP rho);
void function_exit(dyntrace_context_t *context, const SEXP call, const SEXP op,
                   const SEXP rho, const SEXP retval);
void print_entry_info(dyntrace_context_t *context, const SEXP call,
                      const SEXP op, const SEXP rho, function_type fn_type);
void builtin_entry(dyntrace_context_t *context, const SEXP call, const SEXP op,
                   const SEXP rho);
void specialsxp_entry(dyntrace_context_t *context, const SEXP call,
                      const SEXP op, const SEXP rho);
void print_exit_info(dyntrace_context_t *context, const SEXP call,
                     const SEXP op, const SEXP rho, function_type fn_type);
void builtin_exit(dyntrace_context_t *context, const SEXP call, const SEXP op,
                  const SEXP rho, const SEXP retval);
void specialsxp_exit(dyntrace_context_t *context, const SEXP call,
                     const SEXP op, const SEXP rho, const SEXP retval);
void promise_created(dyntrace_context_t *context, const SEXP prom,
                     const SEXP rho);
void promise_force_entry(dyntrace_context_t *context, const SEXP symbol,
                         const SEXP rho);
void promise_force_exit(dyntrace_context_t *context, const SEXP symbol,
                        const SEXP rho, const SEXP val);
void promise_value_lookup(dyntrace_context_t *context, const SEXP symbol,
                          const SEXP rho, const SEXP val);
void promise_expression_lookup(dyntrace_context_t *context, const SEXP prom);
void gc_promise_unmarked(dyntrace_context_t *context, const SEXP promise);
void gc_entry(dyntrace_context_t *context, R_size_t size_needed);
void gc_exit(dyntrace_context_t *context, int gc_count, double vcells,
             double ncells);
void vector_alloc(dyntrace_context_t *context, int sexptype, long length,
                  long bytes, const char *srcref);
void new_environment(dyntrace_context_t *context, const SEXP rho);
void jump_ctxt(dyntrace_context_t *context, const SEXP rho, const SEXP val);
}
#endif /* __PROBES_H__ */
