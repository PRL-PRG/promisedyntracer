#ifndef __TRACER_H__
#define __TRACER_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <Rinternals.h>

SEXP create_dyntracer(SEXP database, SEXP schema, SEXP verbose);

SEXP destroy_dyntracer(SEXP tracer);

#ifdef __cplusplus
}
#endif

#endif /* __TRACER_H__ */
