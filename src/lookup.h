#ifndef __LOOKUP_H__
#define __LOOKUP_H__

#include "stdlibs.h"

enum class lookup_status {
    SUCCESS,
    SUCCESS_ACTIVE_BINDING,
    FAIL_USER_DEFINED_DATABASE,
    FAIL_ENVIRONMENT_IS_NIL,
    FAIL_ARGUMENT_IS_NOT_AN_ENVIRONMENT,
    FAIL_GLOBAL_CACHE
};

std::string lookup_status_to_string(lookup_status);

struct lookup_result {
    lookup_status status;
    SEXP environment;
    SEXP value;
};

lookup_result find_binding_in_environment(const SEXP symbol, const SEXP environment);

#endif /* __LOOKUP_H__ */
