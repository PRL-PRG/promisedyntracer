#ifndef __LOOKUP_H__
#define __LOOKUP_H__

#include "stdlibs.h"

using namespace std;

SEXP find_promise_in_environment(const SEXP symbol, const SEXP rho2);

#endif /* __LOOKUP_H__ */
