#include "lookup.h"

lookup_result find_binding_in_global_cache(const SEXP symbol) {
    return {lookup_status::FAIL_GLOBAL_CACHE, R_GlobalEnv, R_UnboundValue};
}

/*inline*/ lookup_result get_symbol_binding_value(const SEXP symbol,
                                                  const SEXP rho) {
    if (IS_ACTIVE_BINDING(symbol)) {
        SEXP expr = SYMVALUE(symbol);
        return {lookup_status::SUCCESS_ACTIVE_BINDING, rho, expr};
    }
    return {lookup_status::SUCCESS, rho, SYMVALUE(symbol)};
}

/*inline*/ lookup_result get_binding_value(const SEXP frame, const SEXP rho) {
    if (IS_ACTIVE_BINDING(frame)) {
        SEXP expr = CAR(frame);
        return {lookup_status::SUCCESS_ACTIVE_BINDING, rho, expr};
    }
    return {lookup_status::SUCCESS, rho, CAR(frame)};
}

/*inline*/ lookup_result get_hash(int hashcode, const SEXP symbol,
                                  const SEXP rho) {
    const SEXP table = HASHTAB(rho);
    for (SEXP chain = VECTOR_ELT(table, hashcode); chain != R_NilValue;
         chain = CDR(chain)) {
        if (TAG(chain) == symbol)
            return get_binding_value(chain, rho);
    }
    return {lookup_status::SUCCESS, rho, R_UnboundValue};
}

lookup_result find_binding_in_single_environment(const SEXP symbol,
                                                 const SEXP rho) {
    if (TYPEOF(rho) == NILSXP)
        return {lookup_status::FAIL_ENVIRONMENT_IS_NIL, rho, R_UnboundValue};

    if (rho == R_BaseNamespace || rho == R_BaseEnv)
        return get_symbol_binding_value(symbol, rho);

    if (rho == R_EmptyEnv)
        return {lookup_status::SUCCESS, R_EmptyEnv, R_UnboundValue};

    if ((OBJECT(rho)) && inherits((rho), "UserDefinedDatabase"))
        return {lookup_status::FAIL_USER_DEFINED_DATABASE, rho, R_UnboundValue};

    if (HASHTAB(rho) == R_NilValue) {
        SEXP frame = FRAME(rho);
        while (frame != R_NilValue) {
            if (TAG(frame) == symbol)
                return get_binding_value(frame, rho);
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
        return get_hash(hashcode, symbol, rho);
    }

    return {lookup_status::SUCCESS, rho, R_UnboundValue};
}

lookup_result find_binding_in_environment(const SEXP symbol, const SEXP rho2) {
    SEXP rho = rho2;

    if (TYPEOF(rho) == NILSXP)
        return {lookup_status::FAIL_ENVIRONMENT_IS_NIL, rho, R_UnboundValue};

    if (TYPEOF(rho) != ENVSXP) {
        return {lookup_status::FAIL_ARGUMENT_IS_NOT_AN_ENVIRONMENT, rho,
                R_UnboundValue};
    }

#ifdef USE_GLOBAL_CACHE
    while (rho != R_GlobalEnv && rho != R_EmptyEnv) {
#else
    while (rho != R_EmptyEnv) {
#endif
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
        return {lookup_status::SUCCESS, rho, R_UnboundValue};
}

std::string lookup_status_to_string(lookup_status status) {
    switch (status) {
        case lookup_status::FAIL_ARGUMENT_IS_NOT_AN_ENVIRONMENT:
            return "Lookup in environment failed: second argument is not an "
                   "environment";
        case lookup_status::FAIL_ENVIRONMENT_IS_NIL:
            return "Lookup in environment failed: NIL is not a valid "
                   "environment (anymore)";
        case lookup_status::FAIL_GLOBAL_CACHE:
            return "Lookup in environment failed: global cache";
        case lookup_status::FAIL_USER_DEFINED_DATABASE:
            return "Lookup in environment failed: user defined database";
    }
    return "Lookup case not handled, search for this string in codebase";
}
