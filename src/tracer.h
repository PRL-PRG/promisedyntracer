#ifndef PROMISEDYNTRACER_TRACER_H
#define PROMISEDYNTRACER_TRACER_H

#include <Rinternals.h>

#include <Rinternals.h>

#ifdef __cplusplus
extern "C" {
#endif

SEXP create_dyntracer(SEXP trace_filepath, SEXP truncate, SEXP enable_trace,
                      SEXP verbose, SEXP output_dir, SEXP binary,
                      SEXP compression_level, SEXP analysis_switch_env);

SEXP destroy_dyntracer(SEXP tracer);

#ifdef __cplusplus
}
#endif

#endif /* PROMISEDYNTRACER_TRACER_H */
