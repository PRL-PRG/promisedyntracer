#ifndef __TRACER_H__
#define __TRACER_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <Rinternals.h>

SEXP create_dyntracer(SEXP output_dir, SEXP database, SEXP schema,
                      SEXP truncate, SEXP verbose, SEXP enable_analysis);

SEXP destroy_dyntracer(SEXP tracer);

#ifdef __cplusplus
}
#endif

#endif /* __TRACER_H__ */
