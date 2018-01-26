#include "lookup.h"

lookup_result find_binding_in_global_cache(const SEXP symbol) {
    //dyntrace_log_error(
    //    "my_find_in_global_cache, missing implementation, confused");
    return {lookup_status::FAIL_GLOBAL_CACHE, R_UnboundValue};
}

/*inline*/ lookup_result get_symbol_binding_value(const SEXP symbol) {
    if (IS_ACTIVE_BINDING(symbol)) {
        //SEXP expr = LCONS(SYMVALUE(symbol), R_NilValue);
        SEXP expr = SYMVALUE(symbol);
        return {lookup_status::SUCCESS_ACTIVE_BINDING, expr};
    }
    return {lookup_status::SUCCESS, SYMVALUE(symbol)};
    // Removed: if IS_ACTIVE_BINDING(s) ? getActiveValue(SYMVALUE(s))
}

/*inline*/ lookup_result get_binding_value(const SEXP frame) {
    if (IS_ACTIVE_BINDING(frame)) {
        SEXP expr = CAR(frame);
        return {lookup_status::SUCCESS_ACTIVE_BINDING, expr};
    }
    return {lookup_status::SUCCESS, CAR(frame)};
    // Removed: if IS_ACTIVE_BINDING(s) ? getActiveValue(CAR(s))
}

/*inline*/ lookup_result get_hash(int hashcode, SEXP symbol, SEXP table) {
    for (SEXP chain = VECTOR_ELT(table, hashcode); chain != R_NilValue;
         chain = CDR(chain)) {
        if (TAG(chain) == symbol)
            return get_binding_value(chain);
    }
    return {lookup_status::SUCCESS, R_UnboundValue};
}

lookup_result find_binding_in_single_environment(const SEXP symbol, const SEXP rho) {
    if (TYPEOF(rho) == NILSXP)
        return {lookup_status::FAIL_ENVIRONMENT_IS_NIL, R_UnboundValue};

    if (rho == R_BaseNamespace || rho == R_BaseEnv)
        return get_symbol_binding_value(symbol);

    if (rho == R_EmptyEnv)
        return {lookup_status::SUCCESS, R_UnboundValue};

    if ((OBJECT(rho)) && inherits((rho), "UserDefinedDatabase")) // IS_USER_DATABASE(rho)
        return {lookup_status::FAIL_USER_DEFINED_DATABASE, R_UnboundValue};

    if (HASHTAB(rho) == R_NilValue) {
        SEXP frame = FRAME(rho);
        while (frame != R_NilValue) {
            if (TAG(frame) == symbol)
                return get_binding_value(frame);
            frame = CDR(frame);
        }
    }

    // Look up by the hash but without changing anything - the original cached
    // the hash on the SXP.
    if (HASHTAB(rho) != R_NilValue) {
        SEXP print_name = PRINTNAME(symbol);
        int hashcode =
            (!HASHASH(print_name))
                ? (newhashpjw(CHAR(print_name)) % LENGTH(HASHTAB(rho)))
                : (HASHVALUE(print_name) % LENGTH(HASHTAB(rho)));
        return get_hash(hashcode, symbol, HASHTAB(rho));
    }

    return {lookup_status::SUCCESS, R_UnboundValue};
}

lookup_result find_binding_in_environment(const SEXP symbol, const SEXP rho2) {
    SEXP rho = rho2;

    if (TYPEOF(rho) == NILSXP)
        return {lookup_status::FAIL_ENVIRONMENT_IS_NIL, R_UnboundValue};

    if (TYPEOF(rho) != ENVSXP)
        return {lookup_status::FAIL_ARGUMENT_IS_NOT_AN_ENVIRONMENT, R_UnboundValue};

#ifdef USE_GLOBAL_CACHE
    while (rho != R_GlobalEnv && rho != R_EmptyEnv)
#else
    while (rho != R_EmptyEnv)
#endif
    {
        lookup_result r = find_binding_in_single_environment(symbol, rho);

        if (r.status != lookup_status::SUCCESS)
            return r;

        if (r.value != R_UnboundValue)
            return r;

        rho = ENCLOS(rho);
    }
#ifdef USE_GLOBAL_CACHE
    if (rho == R_GlobalEnv)
        return find_promise_in_global_cache(symbol);
    else
#endif
        return {lookup_status::SUCCESS, R_UnboundValue};
}

string lookup_status_to_string(lookup_status status) {
    switch (status) {
        case lookup_status::FAIL_ARGUMENT_IS_NOT_AN_ENVIRONMENT:
            return "Lookup in environment failed: second argument is not an environment";
        case lookup_status::FAIL_ENVIRONMENT_IS_NIL:
            return "Lookup in environment failed: NIL is not a valid environment (anymore)";
        case lookup_status::FAIL_GLOBAL_CACHE:
            return "Lookup in environment failed: global cache";
        case lookup_status::FAIL_USER_DEFINED_DATABASE:
            return "Lookup in environment failed: user defined database";
    }
}