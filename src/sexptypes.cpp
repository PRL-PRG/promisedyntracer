#include "sexptypes.h"
#include "lookup.h"

void get_full_type_inner(SEXP sexp, SEXP rho, full_sexp_type &result,
                         std::set<SEXP> &visited);

void infer_type_from_bytecode(SEXP bc, SEXP rho, full_sexp_type &result,
                              std::set<SEXP> &visited) {

    // retrieve the instructions
    SEXP code = BCODE_CODE(bc);
    BCODE *pc = (BCODE *)INTEGER(code);

    // We ignore pc[0].i. This is the version number.
    // Its not relevant to the analysis.

    // Look up the first opcode
    int opcode = findOp(pc[1].v);

    if (opcode == LDNULL_OP)
        result.push_back((sexp_type)NILSXP);
    else if (opcode == LDTRUE_OP)
        result.push_back((sexp_type)LGLSXP);
    else if (opcode == LDFALSE_OP)
        result.push_back((sexp_type)LGLSXP);
    /* constant (literal) values */
    else if (opcode == LDCONST_OP) {
        /* This only has one argument, the index of the literal in the constant
           pool.
           We return the type of this value by accessing it via the index. */
        int index = pc[2].i;
        SEXP consts = BCODE_CONSTS(bc);
        SEXP value = VECTOR_ELT(consts, index);
        get_full_type_inner(value, rho, result, visited);
    }
    /* lookup value bound to symbol in the environment */
    else if (opcode == GETVAR_OP || opcode == DDVAL_OP) {
        int index = pc[2].i;
        SEXP consts = BCODE_CONSTS(bc);
        SEXP value = VECTOR_ELT(consts, index);
        get_full_type_inner(value, rho, result, visited);
    }
    /* guard for inlined expression (function call) */
    else if (opcode == BASEGUARD_OP) {
        int index = pc[2].i;
        SEXP consts = BCODE_CONSTS(bc);
        SEXP value = VECTOR_ELT(consts, index);
        get_full_type_inner(value, rho, result, visited);
    }
    /* looping function calls - while, repeat, etc. */
    else if (opcode == STARTLOOPCNTXT_OP)
        result.push_back((sexp_type)LANGSXP);
    /* call to a closure */
    else if (opcode == GETFUN_OP)
        result.push_back((sexp_type)LANGSXP);
    /* call to a builtin */
    else if (opcode == GETBUILTIN_OP)
        result.push_back((sexp_type)LANGSXP);
    /* call to internal builtin */
    else if (opcode == GETINTLBUILTIN_OP)
        result.push_back((sexp_type)LANGSXP);
    /* call to special functions */
    else if (opcode == CALLSPECIAL_OP)
        result.push_back((sexp_type)LANGSXP);
    /* closure object */
    else if (opcode == MAKECLOSURE_OP)
        result.push_back((sexp_type)CLOSXP);
    /* print all other opcodes */
    else {
        dyntrace_log_warning("cannot infer type of opcode : %d", opcode);
    }
    return;
}

void get_full_type_inner(SEXP sexp, SEXP rho, full_sexp_type &result,
                         std::set<SEXP> &visited) {
    sexp_type type = static_cast<sexp_type>(TYPEOF(sexp));
    result.push_back(type);

    if (visited.find(sexp) != visited.end()) {
        result.push_back(sexp_type::OMEGA);
        return;
    } else {
        visited.insert(sexp);
    }

    if (type == sexp_type::PROM) {
        get_full_type_inner(PRCODE(sexp), PRENV(sexp), result, visited);
        return;
    }

    // Question... are all BCODEs functions?
    if (type == sexp_type::BCODE) {
        infer_type_from_bytecode(sexp, rho, result, visited);
        //            bool try_to_attach_symbol_value = (rho != R_NilValue) ?
        //            isEnvironment(rho) : false;
        //            if (!try_to_attach_symbol_value) return;
        //
        //            SEXP uncompiled_sexp = BODY_EXPR(sexp);
        //            SEXP underlying_expression =
        //            findVar(PRCODE(uncompiled_sexp),
        //            rho);
        //
        //            if (underlying_expression == R_UnboundValue) return;
        //            if (underlying_expression == R_MissingArg) return;
        //
        //            PROTECT(underlying_expression);
        //            //get_full_type(underlying_expression, rho, result,
        //            visited);
        //           UNPROTECT(1);

        // Rprintf("hi from dbg\n`");
        return;
    }

    if (type == sexp_type::SYM) {
        bool try_to_attach_symbol_value =
            (rho != R_NilValue) ? isEnvironment(rho) : false;
        if (!try_to_attach_symbol_value)
            return;
        /* FIXME - findVar can eval an expression. This can fire another hook,
           leading to spurious data.
                   Reproduced below is a gdb backtrace obtained by running
           `dplyr`.
                   At #7, get_full_type_inner invokes `findVar` which leads to
           `eval` on #2.

           0x00000000004d4b3e in R_execClosure (call=0x94e1db0,
           newrho=0x94e1ec8,
           sysparent=0x1a05f80,
           rho=0x1a05f80, arglist=0x19d6b68, op=0x9534cb0) at eval.c:1612
           1612	    RDT_HOOK(probe_function_entry, call, op, newrho);
           (gdb) bt
           #0  0x00000000004d4b3e in R_execClosure (call=0x94e1db0,
           newrho=0x94e1ec8, sysparent=0x1a05f80,
           rho=0x1a05f80, arglist=0x19d6b68, op=0x9534cb0) at eval.c:1612
           #1  0x00000000004d49e9 in Rf_applyClosure (call=0x94e1db0,
           op=0x9534cb0,
           arglist=0x19d6b68,
           rho=0x1a05f80, suppliedvars=0x19d6b68) at eval.c:1583
           #2  0x00000000004d2db7 in Rf_eval (e=0x94e1db0, rho=0x1a05f80) at
           eval.c:776
           #3  0x00000000004bf6c9 in getActiveValue (fun=0x9534cb0) at
           envir.c:154
           #4  0x00000000004bfa53 in R_HashGet (hashcode=0, symbol=0x54de8f8,
           table=0x1e3a4328) at envir.c:281
           #5  0x00000000004c1781 in Rf_findVarInFrame3 (rho=0x953dd50,
           symbol=0x54de8f8, doGet=TRUE)
           at envir.c:1042
           #6  0x00000000004c1fb1 in Rf_findVar (symbol=0x54de8f8,
           rho=0x953dd50) at
           envir.c:1221
           #7  0x00007f3bcb98893a in
           recorder_t<psql_recorder_t>::get_full_type_inner (
           this=0x7f3bcbbde159 <trace_promises<psql_recorder_t>::rec_impl>,
           sexp=0x54de8f8, rho=0x9520500,
           result=std::vector of length 1, capacity 1 = {...}, visited=std::set
           with
           1 elements = {...})
           at
           /home/aviral/projects/aviral-r-dyntrace/rdt-plugins/promises/src/recorder.h:332
           #8  0x00007f3bcb98542f in recorder_t<psql_recorder_t>::get_full_type
           (
           this=0x7f3bcbbde159 <trace_promises<psql_recorder_t>::rec_impl>,
           promise=0x94e1d78, rho=0x9520500,
           result=std::vector of length 1, capacity 1 = {...})
           at
           /home/aviral/projects/aviral-r-dyntrace/rdt-plugins/promises/src/recorder.h:289
           #9  0x00007f3bcb980aeb in
           recorder_t<psql_recorder_t>::create_promise_get_info (
           this=0x7f3bcbbde159 <trace_promises<psql_recorder_t>::rec_impl>,
           promise=0x94e1d78, rho=0x9520500)
           at
           /home/aviral/projects/aviral-r-dyntrace/rdt-plugins/promises/src/recorder.h:352
         */
        SEXP symbol_points_to = find_promise_in_environment(sexp, rho);

        if (symbol_points_to == R_UnboundValue)
            return;
        if (symbol_points_to == R_MissingArg)
            return;
        // if (TYPEOF(symbol_points_to) == SYMSXP) return;

        get_full_type_inner(symbol_points_to, rho, result, visited);

        return;
    }
}

void get_full_type(SEXP promise, full_sexp_type &result) {
    std::set<SEXP> visited;
    get_full_type_inner(PRCODE(promise), PRENV(promise), result, visited);
}

std::string full_sexp_type_to_string(full_sexp_type type) {
    std::stringstream result;
    bool first = true;
    for (auto iterator = type.begin(); iterator != type.end(); ++iterator) {
        if (first) {
            first = false;
        } else {
            result << "->";
        }
        // result << sexp_type_to_string_short(*iterator);
        result << sexp_type_to_string(*iterator);
    }
    return result.str();
}

std::string full_sexp_type_to_number_string(full_sexp_type type) {
    std::stringstream result;
    bool first = true;
    for (auto iterator = type.begin(); iterator != type.end(); ++iterator) {
        if (first) {
            first = false;
        } else {
            result << ",";
        }
        result << sexp_type_to_SEXPTYPE(*iterator);
    }
    return result.str();
}

std::string sexp_type_to_string(sexp_type s) {
    switch (s) {
        case sexp_type::NIL:
            return "null";
        case sexp_type::SYM:
            return "symbol";
        case sexp_type::LIST:
            return "list";
        case sexp_type::CLOS:
            return "closure";
        case sexp_type::ENV:
            return "environment";
        case sexp_type::PROM:
            return "promise";
        case sexp_type::LANG:
            return "language";
        case sexp_type::SPECIAL:
            return "special";
        case sexp_type::BUILTIN:
            return "built-in";
        case sexp_type::CHAR:
            return "character";
        case sexp_type::LGL:
            return "logical";
        case sexp_type::INT:
            return "integer";
        case sexp_type::REAL:
            return "real";
        case sexp_type::CPLX:
            return "complex";
        case sexp_type::STR:
            return "string";
        case sexp_type::DOT:
            return "dot";
        case sexp_type::ANY:
            return "any";
        case sexp_type::VEC:
            return "vector";
        case sexp_type::EXPR:
            return "expression";
        case sexp_type::BCODE:
            return "byte-code";
        case sexp_type::EXTPTR:
            return "external_pointer";
        case sexp_type::WEAKREF:
            return "weak_reference";
        case sexp_type::RAW:
            return "raw";
        case sexp_type::S4:
            return "s4";
        case sexp_type::OMEGA:
            return "..."; // This one's made up.
        default:
            return "<unknown>";
    }
}
SEXPTYPE sexp_type_to_SEXPTYPE(sexp_type s) {
    switch (s) {
        case sexp_type::NIL:
            return NILSXP;
        case sexp_type::SYM:
            return SYMSXP;
        case sexp_type::LIST:
            return LISTSXP;
        case sexp_type::CLOS:
            return CLOSXP;
        case sexp_type::ENV:
            return ENVSXP;
        case sexp_type::PROM:
            return PROMSXP;
        case sexp_type::LANG:
            return LANGSXP;
        case sexp_type::SPECIAL:
            return SPECIALSXP;
        case sexp_type::BUILTIN:
            return BUILTINSXP;
        case sexp_type::CHAR:
            return CHARSXP;
        case sexp_type::LGL:
            return LGLSXP;
        case sexp_type::INT:
            return INTSXP;
        case sexp_type::REAL:
            return REALSXP;
        case sexp_type::CPLX:
            return CPLXSXP;
        case sexp_type::STR:
            return STRSXP;
        case sexp_type::DOT:
            return DOTSXP;
        case sexp_type::ANY:
            return ANYSXP;
        case sexp_type::VEC:
            return VECSXP;
        case sexp_type::EXPR:
            return EXPRSXP;
        case sexp_type::BCODE:
            return BCODESXP;
        case sexp_type::EXTPTR:
            return EXTPTRSXP;
        case sexp_type::WEAKREF:
            return WEAKREFSXP;
        case sexp_type::RAW:
            return RAWSXP;
        case sexp_type::S4:
            return S4SXP;
        case sexp_type::OMEGA:
            return 69; // This one's made up.
        default:
            return -1;
    }
}

std::string sexp_type_to_string_short(sexp_type s) {
    switch (s) {
        case sexp_type::NIL:
            return "NIL";
        case sexp_type::SYM:
            return "SYM";
        case sexp_type::LIST:
            return "LIST";
        case sexp_type::CLOS:
            return "CLOS";
        case sexp_type::ENV:
            return "ENV";
        case sexp_type::PROM:
            return "PROM";
        case sexp_type::LANG:
            return "LANG";
        case sexp_type::SPECIAL:
            return "SPECIAL";
        case sexp_type::BUILTIN:
            return "BUILTIN";
        case sexp_type::CHAR:
            return "CHAR";
        case sexp_type::LGL:
            return "LGL";
        case sexp_type::INT:
            return "INT";
        case sexp_type::REAL:
            return "REAL";
        case sexp_type::CPLX:
            return "CPL";
        case sexp_type::STR:
            return "STR";
        case sexp_type::DOT:
            return "DOT";
        case sexp_type::ANY:
            return "ANY";
        case sexp_type::VEC:
            return "VEC";
        case sexp_type::EXPR:
            return "EXPR";
        case sexp_type::BCODE:
            return "BCODE";
        case sexp_type::EXTPTR:
            return "EXTPTR";
        case sexp_type::WEAKREF:
            return "WEAKREF";
        case sexp_type::RAW:
            return "RAW";
        case sexp_type::S4:
            return "S4";
        case sexp_type::OMEGA:
            return "..."; // This one's made up.
        default:
            return "NA";
    }
}
