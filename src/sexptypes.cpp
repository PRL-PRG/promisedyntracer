#include "sexptypes.h"
#include "lookup.h"

const sexptype_t OMEGASXP = 100000;
const sexptype_t ACTIVESXP = 100001;
const sexptype_t UNBOUNDSXP = 100002;

std::string sexptype_to_string(sexptype_t sexptype) {
    switch (sexptype) {
        case NILSXP:
            return "Null";
        case UNBOUNDSXP:
            return "Unbound";
        case LANGSXP:
            return "Function Call";
        case NEWSXP:
            return "New";
        case FREESXP:
            return "Free";
        case FUNSXP:
            return "Closure or Builtin";
        case OMEGASXP:
            return "Omega";
        case ACTIVESXP:
            return "Active binding";
        default:
            std::string str(type2char(sexptype));
            str[0] = std::toupper(str[0]);
            return str;
    }
}

std::string value_type_to_string(SEXP value) {
    if (value == R_UnboundValue) {
        return sexptype_to_string(UNBOUNDSXP);
    }
    return sexptype_to_string(static_cast<sexptype_t>(TYPEOF(value)));
}

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
        result.push_back((sexptype_t)NILSXP);
    else if (opcode == LDTRUE_OP)
        result.push_back((sexptype_t)LGLSXP);
    else if (opcode == LDFALSE_OP)
        result.push_back((sexptype_t)LGLSXP);
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
        result.push_back((sexptype_t)LANGSXP);
    /* call to a closure */
    else if (opcode == GETFUN_OP)
        result.push_back((sexptype_t)LANGSXP);
    /* call to a builtin */
    else if (opcode == GETBUILTIN_OP)
        result.push_back((sexptype_t)LANGSXP);
    /* call to internal builtin */
    else if (opcode == GETINTLBUILTIN_OP)
        result.push_back((sexptype_t)LANGSXP);
    /* call to special functions */
    else if (opcode == CALLSPECIAL_OP)
        result.push_back((sexptype_t)LANGSXP);
    /* closure object */
    else if (opcode == MAKECLOSURE_OP)
        result.push_back((sexptype_t)CLOSXP);
    /* print all other opcodes */
    else {
        dyntrace_log_warning("cannot infer type of opcode : %d", opcode);
    }
    return;
}

void get_full_type_inner(SEXP sexp, SEXP rho, full_sexp_type &result,
                         std::set<SEXP> &visited) {
    sexptype_t type = static_cast<sexptype_t>(TYPEOF(sexp));
    result.push_back(type);

    if (type == PROMSXP && visited.find(sexp) != visited.end()) {
        result.push_back((sexptype_t)OMEGASXP);
        return;
    } else {
        visited.insert(sexp);
    }

    if (type == (sexptype_t)PROMSXP) {
        get_full_type_inner(PRCODE(sexp), PRENV(sexp), result, visited);
        return;
    }

    if (type == (sexptype_t)BCODESXP) {
        infer_type_from_bytecode(sexp, rho, result, visited);
        return;
    }

    if (type == (sexptype_t)SYMSXP) {
        lookup_result r = find_binding_in_environment(sexp, rho);

        switch (r.status) {
            case lookup_status::SUCCESS: {
                if (r.value == R_UnboundValue || r.value == R_MissingArg)
                    return;

                if (r.value == sexp && r.environment == rho) {
                    result.push_back((sexptype_t)OMEGASXP);
                    return;
                }

                get_full_type_inner(r.value, r.environment, result, visited);

                return;
            }

            case lookup_status::SUCCESS_ACTIVE_BINDING: {
                result.push_back((sexptype_t)ACTIVESXP);

                SEXP symbol_points_to = r.value;
                if (symbol_points_to == R_UnboundValue ||
                    symbol_points_to == R_MissingArg)
                    return;

                /* TODO in order to proceed to explore active bindings,
                   we'd need the environment in which it's defined */
                // get_full_type_inner(symbol_points_to, rho, result, visited);
                result.push_back(static_cast<sexptype_t>(TYPEOF(r.value)));
                return;
            }

            case lookup_status::FAIL_ENVIRONMENT_IS_NIL:
                /* ignore this */
                return;

            default: {
                std::string msg = lookup_status_to_string(r.status);
                dyntrace_log_warning("%s", msg.c_str());
                // result.push_back(sexptype_t::OMEGA);
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
        result << sexptype_to_string(*iterator);
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
            result << "->";
        }
        result << *iterator;
    }
    return result.str();
}

std::string infer_sexptype(SEXP promise) {
    std::vector<sexptype_t> type_trace;
    std::string type_string = "";
    get_full_type(promise, type_trace);
    return full_sexp_type_to_string(type_trace);
}
