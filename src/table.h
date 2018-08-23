#ifndef PROMISEDYNTRACER_TABLE_H
#define PROMISEDYNTRACER_TABLE_H

#include "DataTableStream.h"
#include <Rinternals.h>
#include <string>
#include <vector>

DataTableStream *create_data_table(const std::string &table_filepath,
                                   const std::vector<std::string> &column_names,
                                   bool truncate, bool binary = true,
                                   int compression_level = 0);

#ifdef __cplusplus
extern "C" {
#endif

SEXP write_data_table(SEXP data_frame, SEXP table_filepath, SEXP truncate,
                      SEXP binary, SEXP compression_level);

SEXP read_data_table(SEXP table_filepath, SEXP binary, SEXP compression_level);

#ifdef __cplusplus
}
#endif

#endif /* PROMISEDYNTRACER_TABLE_H */
