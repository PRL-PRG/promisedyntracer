#include <R_ext/Rdynload.h>
#include <R_ext/Visibility.h>
#include "tracer.h"

static const R_CallMethodDef CallEntries[] = {
    {"create_dyntracer", (DL_FUNC) &create_dyntracer, 5},
    {"destroy_dyntracer", (DL_FUNC) &destroy_dyntracer, 1},
    {NULL, NULL, 0}
};

void attribute_visible R_init_promisedyntracer(DllInfo *dll) {
    R_registerRoutines(dll, NULL, CallEntries, NULL, NULL);
    R_useDynamicSymbols(dll, FALSE);
    R_forceSymbols(dll, TRUE);
}
