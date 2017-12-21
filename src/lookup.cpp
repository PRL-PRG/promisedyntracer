#include "lookup.h"

SEXP find_promise_in_global_cache(const SEXP symbol) {
    Rf_error(_("DYNTRACE: my_find_in_global_cache, missing implementation, confused"));
}

inline SEXP get_symbol_binding_value(const SEXP symbol) {
    if (IS_ACTIVE_BINDING(symbol))
        Rf_error(_("DYNTRACE: encountered ACTIVE BINDING symbol, missing implementation, confused"));
    return SYMVALUE(symbol); // Removed: if IS_ACTIVE_BINDING(s) ? getActiveValue(SYMVALUE(s))
}

inline SEXP get_binding_value(const SEXP frame) {
    if (IS_ACTIVE_BINDING(frame))
        Rf_error(_("DYNTRACE: encountered ACTIVE BINDING frame, missing implementation, confused"));
    return CAR(frame); // Removed: if IS_ACTIVE_BINDING(s) ? getActiveValue(CAR(s))
}

inline SEXP get_hash(int hashcode, SEXP symbol, SEXP table)
{
    for (SEXP chain = VECTOR_ELT(table, hashcode);
         chain != R_NilValue; chain = CDR(chain)) {
        if (TAG(chain) == symbol)
            return get_binding_value(chain);
    }
    return R_UnboundValue;
}

SEXP find_promise_in_single_environment(const SEXP symbol, const SEXP rho) {
    if (TYPEOF(rho) == NILSXP)
        Rf_error(_("DYNTRACE: encountered NILSXP as environment"));

    if (rho == R_BaseNamespace || rho == R_BaseEnv)
        return get_symbol_binding_value(symbol);

    if (rho == R_EmptyEnv)
        return R_UnboundValue;

    if ((OBJECT(rho)) && inherits((rho), "UserDefinedDatabase")) // (IS_USER_DATABASE(rho))
        Rf_error(_("DYNTRACE: encountered UserDefinedDatabase class environment, missing implementation, confused"));

    if (HASHTAB(rho) == R_NilValue) {
        SEXP frame = FRAME(rho);
        while (frame != R_NilValue) {
            if (TAG(frame) == symbol)
                return get_binding_value(frame);
            frame = CDR(frame);
        }
    }

    // Look up by the hash but without changing anything - the original cached the hash on the SXP.
    if (HASHTAB(rho) != R_NilValue) {
        SEXP print_name = PRINTNAME(symbol);
        int hashcode = (!HASHASH(print_name)) ?
                       (newhashpjw(CHAR(print_name)) % LENGTH(HASHTAB(rho))) :
                       (HASHVALUE(print_name) % LENGTH(HASHTAB(rho)));
        return get_hash(hashcode, symbol, HASHTAB(rho));
    }

    return R_UnboundValue;
}

SEXP find_promise_in_environment(const SEXP symbol, const SEXP rho2) {
    SEXP rho = rho2;

    if (TYPEOF(rho) == NILSXP)
        Rf_error(_("DYNTRACE: encountered NILSXP as environment"));

    if (TYPEOF(rho) != ENVSXP)
        Rf_error(_("DYNTRACE: argument to '%s' is not an environment"));

#ifdef USE_GLOBAL_CACHE
    while (rho != R_GlobalEnv && rho != R_EmptyEnv)
#else
    while (rho != R_EmptyEnv)
#endif
    {
        SEXP v = find_promise_in_single_environment(symbol, rho);
        if (v != R_UnboundValue)
            return v;
        rho = ENCLOS(rho);
    }
#ifdef USE_GLOBAL_CACHE
    if (rho == R_GlobalEnv)
        return find_promise_in_global_cache(symbol);
    else
#endif
    return R_UnboundValue;
}
