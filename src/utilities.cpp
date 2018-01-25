#include "utilities.h"
#include "base64.h"
#include "lookup.h"

size_t SQLITE3_ERROR_MESSAGE_BUFFER_SIZE = 1000;
size_t SQLITE3_EXPANDED_SQL_BUFFER_SIZE = 2000;

int get_file_size(std::ifstream &file) {
    int position = file.tellg();
    file.seekg(0, std::ios_base::end);
    int length = file.tellg();
    file.seekg(position, std::ios_base::beg);
    return length;
}

std::string readfile(std::ifstream &file) {
    std::string contents;
    file.seekg(0, std::ios::end);
    contents.reserve(file.tellg());
    file.seekg(0, std::ios::beg);
    contents.assign(std::istreambuf_iterator<char>(file),
                    std::istreambuf_iterator<char>());
    return contents;
}

bool file_exists(const std::string &filepath) {
    return std::ifstream(filepath).good();
}

char *copy_string(char *destination, const char *source, size_t buffer_size) {
    size_t l = strlen(source);
    if (l >= buffer_size) {
        strncpy(destination, source, buffer_size - 1);
        destination[buffer_size - 1] = '\0';
    } else {
        strcpy(destination, source);
    }
    return destination;
}

bool sexp_to_bool(SEXP value) { return LOGICAL(value)[0] == TRUE; }

int sexp_to_int(SEXP value) { return (int)*REAL(value); }

std::string sexp_to_string(SEXP value) {
    return std::string(CHAR(STRING_ELT(value, 0)));
}

const char *get_name(SEXP sexp) {
    const char *s = NULL;

    switch (TYPEOF(sexp)) {
        case CHARSXP:
            s = CHAR(sexp);
            break;
        case LANGSXP:
            s = get_name(CAR(sexp));
            break;
        case BUILTINSXP:
        case SPECIALSXP:
            s = CHAR(PRIMNAME(sexp));
            break;
        case SYMSXP:
            s = CHAR(PRINTNAME(sexp));
            break;
    }

    return s;
}

static int get_lineno(SEXP srcref) {
    if (srcref && srcref != R_NilValue) {

        if (TYPEOF(srcref) == VECSXP) {
            srcref = VECTOR_ELT(srcref, 0);
        }

        return asInteger(srcref);
    }

    return -1;
}

static int get_colno(SEXP srcref) {
    if (srcref && srcref != R_NilValue) {

        if (TYPEOF(srcref) == VECSXP) {
            srcref = VECTOR_ELT(srcref, 0);
        }

        //        INTEGER(val)[0] = lloc->first_line;
        //        INTEGER(val)[1] = lloc->first_byte;
        //        INTEGER(val)[2] = lloc->last_line;
        //        INTEGER(val)[3] = lloc->last_byte;
        //        INTEGER(val)[4] = lloc->first_column;
        //        INTEGER(val)[5] = lloc->last_column;
        //        INTEGER(val)[6] = lloc->first_parsed;
        //        INTEGER(val)[7] = lloc->last_parsed;

        if (TYPEOF(srcref) == INTSXP) {
            // lineno = INTEGER(srcref)[0];
            return INTEGER(srcref)[4];
        } else {
            // This should never happen, right?
            return -1;
        }
    }

    return -1;
}

#include <Rinternals.h>
static const char *get_filename(SEXP srcref) {
    if (srcref && srcref != R_NilValue) {
        if (TYPEOF(srcref) == VECSXP)
            srcref = VECTOR_ELT(srcref, 0);
        SEXP srcfile = getAttrib(srcref, R_SrcfileSymbol);
        if (TYPEOF(srcfile) == ENVSXP) {
            SEXP filename = find_promise_in_environment(install("filename"), srcfile);
            if (isString(filename) && Rf_length(filename)) {
                return CHAR(STRING_ELT(filename, 0));
            }
        }
    }

    return NULL;
}

char *get_callsite(int how_far_in_the_past) {
    SEXP srcref = R_GetCurrentSrcref(how_far_in_the_past);
    const char *filename = get_filename(srcref);
    int lineno = get_lineno(srcref);
    int colno = get_colno(srcref);
    char *location = NULL;

    if (filename) {
        if (strlen(filename) > 0) {
            asprintf(&location, "%s:%d,%d", filename, lineno, colno);
        } else {
            asprintf(&location, "<console>:%d,%d", lineno, colno);
        }
    }

    return location;
}

char *get_location(SEXP op) {
    SEXP srcref = getAttrib(op, R_SrcrefSymbol);
    const char *filename = get_filename(srcref);
    int lineno = get_lineno(srcref);
    int colno = get_colno(srcref);
    char *location = NULL;

    if (filename) {
        if (strlen(filename) > 0) {
            asprintf(&location, "%s:%d,%d", filename, lineno, colno);
        } else {
            asprintf(&location, "<console>:%d,%d", lineno, colno);
        }
    }

    return location;
}

const char *get_call(SEXP call) {
    return serialize_sexp(call);
    // return CHAR(STRING_ELT(deparse1s(call), 0)); // we don't use this in the
    // promise tracer.
}

int is_byte_compiled(SEXP op) {
    SEXP body = BODY(op);
    return TYPEOF(body) == BCODESXP;
}

std::string get_expression(SEXP e) {
    char *expr = serialize_sexp(e);
    assert(expr != NULL);
    std::string expression(expr);
    free(expr);
    return expression;
}

// returns a monotonic timestamp in microseconds
uint64_t timestamp() {
    uint64_t t;
// The __MACH__ bit is from http://stackoverflow.com/a/6725161/6846474
#if !defined(HAVE_CLOCK_GETTIME) && defined(__MACH__)
    clock_serv_t cclock;
    mach_timespec_t mts;
    host_get_clock_service(mach_host_self(), SYSTEM_CLOCK, &cclock);
    clock_get_time(cclock, &mts);
    mach_port_deallocate(mach_task_self(), cclock);
    t = mts.tv_sec * 1e9 + mts.tv_nsec;
#else
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    t = ts.tv_sec * 1e9 + ts.tv_nsec;
#endif
    return t;
}

const char *get_ns_name(SEXP op) {
    SEXP env = CLOENV(op);
    SEXP spec = R_NamespaceEnvSpec(env);

    if (spec != R_NilValue) {
        if (TYPEOF(spec) == STRSXP && LENGTH(spec) > 0) {
            return CHAR(STRING_ELT(spec, 0));
        } else if (TYPEOF(spec) == CHARSXP) {
            return CHAR(spec);
        }
    }

    return NULL;
}

#if OPENSSL_VERSION_NUMBER < 0x10100000L

#include <string.h>
#include <openssl/engine.h>

static void *OPENSSL_zalloc(size_t num) {
    void *ret = OPENSSL_malloc(num);

    if (ret != NULL)
        memset(ret, 0, num);

    return ret;
}

EVP_MD_CTX *EVP_MD_CTX_new(void)
{
    return OPENSSL_zalloc(sizeof(EVP_MD_CTX));
}

void EVP_MD_CTX_free(EVP_MD_CTX *ctx)
{
    EVP_MD_CTX_cleanup(ctx);
    OPENSSL_free(ctx);
}
#endif

std::string compute_hash(const char *data) {
    EVP_MD_CTX *mdctx = EVP_MD_CTX_new();
    const EVP_MD *md = EVP_md5();
    unsigned char md_value[EVP_MAX_MD_SIZE];
    unsigned int md_len = 0;
    EVP_MD_CTX_init(mdctx);
    EVP_DigestInit_ex(mdctx, md, NULL);
    EVP_DigestUpdate(mdctx, data, strlen(data));
    EVP_DigestFinal_ex(mdctx, md_value, &md_len);
    EVP_MD_CTX_free(mdctx);

    return base64_encode(reinterpret_cast<const unsigned char *>(md_value),
                         md_len);
}

const char *remove_null(const char *value) { return value ? value : ""; }

std::string clock_ticks_to_string(clock_t ticks) {
    return std::to_string((double)ticks / CLOCKS_PER_SEC);
}