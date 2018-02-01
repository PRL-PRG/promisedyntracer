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

    if (type == sexp_type::BCODE) {
        infer_type_from_bytecode(sexp, rho, result, visited);
        return;
    }

    if (type == sexp_type::SYM) {
        bool try_to_attach_symbol_value =
                (rho != R_NilValue) ? isEnvironment(rho) : false;
        if (!try_to_attach_symbol_value)
            return;

        lookup_result r = find_binding_in_environment(sexp, rho);

        switch (r.status) {
            case lookup_status::SUCCESS: {
                SEXP symbol_points_to = r.value;
                if (symbol_points_to == R_UnboundValue
                    || symbol_points_to == R_MissingArg)
                    return;

                get_full_type_inner(symbol_points_to, rho, result, visited);

                return;
            }

            case lookup_status::SUCCESS_ACTIVE_BINDING: {
                result.push_back(sexp_type::ACTIVE_BINDING);

                SEXP symbol_points_to = r.value;
                if (symbol_points_to == R_UnboundValue
                    || symbol_points_to == R_MissingArg)
                    return;

                /* TODO in order to proceed to explore active bindings,
                   we'd need the environment in which it's defined */
                // get_full_type_inner(symbol_points_to, rho, result, visited);
                result.push_back(static_cast<sexp_type>(TYPEOF(r.value)));
                return;
            }

            default: {
                string msg = lookup_status_to_string(r.status);
                dyntrace_log_warning("%s", msg.c_str());
                result.push_back(sexp_type::OMEGA);
                return;
            }
        }
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
