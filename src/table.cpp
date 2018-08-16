#include "table.h"
#include "BinaryDataTableStream.h"
#include "TextDataTableStream.h"
#include "utilities.h"

DataTableStream *create_data_table(const std::string &table_filepath,
                                   const std::vector<std::string> &column_names,
                                   bool truncate, bool binary,
                                   int compression_level) {
    std::string extension = compression_level == 0 ? "" : ".zst";
    DataTableStream *stream = nullptr;
    if (binary) {
        stream = new BinaryDataTableStream(table_filepath + ".bin" + extension,
                                           column_names, truncate,
                                           compression_level);
    } else {
        stream =
            new TextDataTableStream(table_filepath + ".csv" + extension,
                                    column_names, truncate, compression_level);
    }
    return stream;
}

SEXP write_data_table(SEXP data_frame, SEXP table_filepath, SEXP truncate,
                      SEXP binary, SEXP compression_level) {

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

    std::size_t row_count = column_count == 0 ? 0 : LENGTH(columns[0]);

    DataTableStream *stream = create_data_table(
        sexp_to_string(table_filepath), column_names, sexp_to_bool(truncate),
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

static SEXP read_text_data_table(const std::string &filepath,
                                 int compression_level) {
    return R_NilValue;
}

int parse_integer(const char *buffer, const char **end, std::size_t bytes = 4) {
    int value = 0;
    std::memcpy(&value, buffer, bytes);
    *end = buffer + bytes;
    return value;
};

double parse_real(const char *buffer, const char **end) {
    double value = 0.0;
    std::memcpy(&value, buffer, sizeof(double));
    *end = buffer + sizeof(double);
    return value;
};

int parse_logical(const char *buffer, const char **end) {
    int value = 0;
    std::memcpy(&value, buffer, sizeof(bool));
    *end = buffer + sizeof(bool);
    return value;
};

SEXPTYPE parse_sexptype(const char *buffer, const char **end) {
    std::uint32_t value = 0;
    std::memcpy(&value, buffer, sizeof(std::uint32_t));
    *end = buffer + sizeof(std::uint32_t);
    return value;
};

SEXP parse_character(const char *buffer, const char **end, char **dest,
                     std::size_t *dest_size) {

    std::uint32_t size = parse_integer(buffer, end, sizeof(std::uint32_t));

    if (size >= *dest_size) {
        free(*dest);
        *dest_size = 2 * size;
        *dest = static_cast<char *>(malloc_or_die(*dest_size));
    }

    std::memcpy(*dest, *end, size);
    *end = *end + size;
    (*dest)[size] = '\0';

    return mkChar(*dest);
};

struct data_frame_t {
    SEXP object;
    std::size_t row_count;
    std::size_t column_count;
    std::vector<SEXP> columns;
    std::vector<DataTableStream::column_type_t> column_types;
};

static data_frame_t read_header(const char *buffer, const char **end) {

    data_frame_t data_frame{nullptr, 0, 0, {}, {}};
    std::size_t character_size = 1024 * 1024;
    char *character_value = static_cast<char *>(malloc(character_size));

    int row_count = parse_integer(buffer, end);
    int column_count = parse_integer(*end, end);

    data_frame.row_count = row_count;
    data_frame.column_count = column_count;

    std::cerr << "Rows: " << row_count << std::endl;
    std::cerr << "Cols: " << column_count << std::endl;

    data_frame.object = PROTECT(allocVector(VECSXP, column_count));
    SEXP column_names = PROTECT(allocVector(STRSXP, column_count));
    SEXP row_names = PROTECT(allocVector(STRSXP, row_count));

    data_frame.columns.reserve(column_count);
    data_frame.column_types.reserve(column_count);

    for (int column_index = 0; column_index < column_count; ++column_index) {
        SEXP name =
            parse_character(*end, end, &character_value, &character_size);
        std::cerr << "Read: " << CHAR(name) << "\n";
        SET_STRING_ELT(column_names, column_index, name);
        SEXPTYPE sexptype = parse_sexptype(*end, end);
        uint32_t size = parse_integer(*end, end);
        data_frame.column_types.push_back({sexptype, size});
        SEXP column = PROTECT(allocVector(sexptype, row_count));
        data_frame.columns.push_back(column);
        SET_VECTOR_ELT(data_frame.object, column_index, column);
    }

    for (int row_index = 0; row_index < row_count; ++row_index) {
        sprintf(character_value, "%d", row_index + 1);
        SET_STRING_ELT(row_names, row_index, mkChar(character_value));
    }

    setAttrib(data_frame.object, R_RowNamesSymbol, row_names);
    setAttrib(data_frame.object, R_NamesSymbol, column_names);
    classgets(data_frame.object, mkString("data.frame"));

    free(character_value);
    UNPROTECT(3);
    return data_frame;
}

static SEXP read_binary_data_table(const std::string &filepath,
                                   int compression_level) {
    if (compression_level != 0) {
        return R_NilValue;
    }

    auto const[buf, buffer_size] = map_to_memory(filepath);
    const char *buffer = static_cast<const char *>(buf);
    const char *const end_of_buffer = buffer + buffer_size;
    const char *end = nullptr;
    data_frame_t data_frame{read_header(buffer, &end)};
    std::size_t character_size = 1024 * 1024;
    char *character_value = static_cast<char *>(malloc(character_size));

    int row_index = 0;
    int column_index = 0;
    while (row_index < data_frame.row_count) {
        switch (data_frame.column_types[column_index].first) {

            case LGLSXP:
                LOGICAL(data_frame.columns[column_index])
                [row_index] = parse_logical(end, &end);
                break;

            case INTSXP:
                INTEGER(data_frame.columns[column_index])
                [row_index] = parse_integer(
                    end, &end, data_frame.column_types[column_index].second);
                break;

            case REALSXP:
                REAL(data_frame.columns[column_index])
                [row_index] = parse_real(end, &end);
                break;

            case STRSXP:
                SET_STRING_ELT(data_frame.columns[column_index], row_index,
                               parse_character(end, &end, &character_value,
                                               &character_size));
                break;

            default:
                Rf_error("unhandled column type %d of column %d in %s ",
                         data_frame.column_types[column_index].first,
                         column_index, filepath.c_str());
        }

        ++column_index;
        if (column_index == data_frame.column_count) {
            column_index = 0;
            ++row_index;
        }
    }

    UNPROTECT(data_frame.column_count);
    std::free(character_value);
    return data_frame.object;
}

SEXP read_data_table(SEXP table_filepath, SEXP binary, SEXP compression_level) {
    const std::string filepath_unwrapped = sexp_to_string(table_filepath);
    bool binary_unwrapped = sexp_to_bool(binary);
    int compression_level_unwrapped = sexp_to_int(compression_level);
    return (binary_unwrapped
                ? read_binary_data_table(filepath_unwrapped,
                                         compression_level_unwrapped)
                : read_text_data_table(filepath_unwrapped,
                                       compression_level_unwrapped));
}
