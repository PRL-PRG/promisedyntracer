#ifndef __TRACER_H__
#define __TRACER_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <Rinternals.h>

SEXP create_dyntracer(SEXP trace_filepath, SEXP truncate, SEXP verbose,
                      SEXP output_dir, SEXP enable_analysis);

SEXP destroy_dyntracer(SEXP tracer);

#ifdef __cplusplus
}
#endif

#endif /* __TRACER_H__ */
