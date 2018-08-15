#include "table.h"
#include "BinaryDataTableStream.h"
#include "TextDataTableStream.h"
#include "utilities.h"

DataTableStream *create_data_table(const std::string &table_filepath,
                                   const std::vector<std::string> &column_names,
                                   bool binary, int compression_level) {
    std::string extension = compression_level == 0 ? "" : ".zstd";
    DataTableStream *stream = nullptr;
    if (binary) {
        stream = new BinaryDataTableStream(table_filepath + ".bin" + extension,
                                           column_names, compression_level);
    } else {
        stream = new TextDataTableStream(table_filepath + ".csv" + extension,
                                         column_names, compression_level);
    }
    return stream;
}

SEXP write_data_table(SEXP data_frame, SEXP table_filepath, SEXP binary,
                      SEXP compression_level) {

    std::size_t column_count = LENGTH(data_frame);
    std::vector<std::string> column_names{column_count, ""};
    std::vector<SEXP> columns{column_count, R_NilValue};
    std::vector<DataTableStream::column_type_t> column_types{column_count,
                                                             {NILSXP, 0}};
    SEXP column_name_list = getAttrib(data_frame, R_NamesSymbol);
    SEXP column_name_str = R_NilValue;

    for (int column_index = 0; column_index < column_count; ++column_index) {
        columns[column_index] = VECTOR_ELT(data_frame, column_index);
        column_types[column_index] =
            std::make_pair((SEXPTYPE)TYPEOF(columns[column_index]), 0);

        if (column_name_list == R_NilValue) {
            continue;
        }

        column_name_str = STRING_ELT(column_name_list, column_index);

        if (TYPEOF(column_name_str) == CHARSXP) {
            column_names[column_index] = CHAR(column_name_str);
        }
    }

    std::size_t row_count = LENGTH(columns[0]);

    DataTableStream *stream =
        create_data_table(sexp_to_string(table_filepath), column_names,
                          sexp_to_bool(binary), sexp_to_int(compression_level));

    for (int row_index = 0; row_index < row_count; ++row_index) {
        for (int column_index = 0; column_index < column_count;
             ++column_index) {
            switch (column_types[column_index].first) {

                case LGLSXP:
                    stream->write_column(static_cast<bool>(
                        LOGICAL(columns[column_index])[row_index]));
                    break;

                case INTSXP:
                    stream->write_column(static_cast<int>(
                        INTEGER(columns[column_index])[row_index]));
                    break;

                case REALSXP:
                    stream->write_column(static_cast<double>(
                        REAL(columns[column_index])[row_index]));
                    break;

                case STRSXP:
                    stream->write_column(std::string(
                        CHAR(STRING_ELT(columns[column_index], row_index))));
                    break;
            }
        }
    }

    delete stream;
    return R_NilValue;
}

SEXP read_data_table(SEXP table_filepath, SEXP binary, SEXP compression_level) {
    // const std::string filepath = sexp_to_string(input_filepath);
    // bool binary_ = sexp_to_bool(binary);
    // int compression_level_ = sexp_to_int(compression_level);
    // if (!binary_ && compression_level == 0) {
    //     return Rf_eval(lang3(install("read.table"), function,
    //                          ScalarString(mkChar("line"))),
    //                    environment);
    // }

    // std::cerr << "filename: " << filepath << std::endl;
    // auto const[buf, buffer_size] = map_to_memory(filepath);
    // const char *buffer = static_cast<const char *>(buf);
    // const char *const end_of_buffer = buffer + buffer_size;

    // if (buffer != end_of_buffer) {
    //     Rf_error("end of buffer not reached after parsing %s",
    //              filepath.c_str());
    // }

    // setAttrib(data_frame, R_NamesSymbol, column_names);
    // UNPROTECT(2);
    // return data_frame;
}
