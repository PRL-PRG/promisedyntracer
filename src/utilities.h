#ifndef __UTILITIES_H__
#define __UTILITIES_H__

#include "stdlibs.h"
#include <openssl/evp.h>

int get_file_size(std::ifstream &file);

std::string readfile(const std::string &filepath);

bool file_exists(const std::string &filepath);

bool sexp_to_bool(SEXP value);

int sexp_to_int(SEXP value);

std::string sexp_to_string(SEXP value);

template <typename T>
std::underlying_type_t<T> to_underlying_type(const T &enum_val) {
    return static_cast<std::underlying_type_t<T>>(enum_val);
}

std::string compute_hash(const char *data);
const char *get_ns_name(SEXP op);
const char *get_name(SEXP call);
char *get_location(SEXP op);
char *get_callsite(int);
const char *get_call(SEXP call);
int is_byte_compiled(SEXP op);
// char *to_string(SEXP var);
const char *get_expression(SEXP e);

#endif /* __UTILITIES_H__ */
