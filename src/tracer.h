#ifndef __TRACER_H__
#define __TRACER_H__

#include <Rinternals.h>

#ifdef __cplusplus
extern "C" {
#endif

SEXP create_dyntracer(SEXP trace_filepath, SEXP truncate, SEXP enable_trace,
                      SEXP verbose, SEXP output_dir, SEXP analysis_switch_env);

SEXP destroy_dyntracer(SEXP tracer);

#ifdef __cplusplus
}
#endif

#endif /* __TRACER_H__ */
