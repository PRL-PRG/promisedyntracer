#ifndef __UTILITIES_H__
#define __UTILITIES_H__

#include "stdlibs.h"
#include "AnalysisSwitch.h"
#include <openssl/evp.h>

int get_file_size(std::ifstream &file);

std::string readfile(std::ifstream &file);

bool file_exists(const std::string &filepath);

char *copy_string(char *destination, const char *source, size_t buffer_size);

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
std::string get_definition_location_cpp(SEXP op);
std::string get_callsite_cpp(int);
const char *get_call(SEXP call);
int is_byte_compiled(SEXP op);
// char *to_string(SEXP var);
std::string get_expression(SEXP e);
const char *remove_null(const char *value);
std::string clock_ticks_to_string(clock_t ticks);
AnalysisSwitch to_analysis_switch(SEXP env);
#endif /* __UTILITIES_H__ */
