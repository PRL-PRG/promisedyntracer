#ifndef PROMISEDYNTRACER_BINARY_DATA_TABLE_STREAM_H
#define PROMISEDYNTRACER_BINARY_DATA_TABLE_STREAM_H

#include "DataTableStream.h"

class BinaryDataTableStream : public DataTableStream {
  public:
    explicit BinaryDataTableStream(const std::string &table_filepath,
                                   const std::vector<std::string> &column_names,
                                   bool truncate, int compression_level)
        : DataTableStream(table_filepath, column_names, truncate,
                          compression_level),
          column_types_{column_names.size(), {NILSXP, 0}} {

        std::size_t header_buffer_size = 8;

        for (const auto &column_name : column_names) {
            header_buffer_size += 4 + column_name.size() + 4 + 4;
        }

        fill(0, header_buffer_size);
        flush();
    }

    void store_or_check_column_type(const column_type_t &column_type) {
        if (get_current_row_index() == 0) {
            column_types_[get_current_column_index()] = column_type;
        } else if (column_types_[get_current_column_index()] != column_type) {
            std::fprintf(
                stderr,
                "column type mismatch: expected %s of %d bytes at column "
                "%lo of file %s",
                sexptype_to_string(column_type.first).c_str(),
                column_type.second, get_current_column_index(),
                get_filepath().c_str());
            exit(EXIT_FAILURE);
        }
    }

    const std::vector<column_type_t> &get_column_types() const {
        return column_types_;
    }

    const column_type_t &get_column_type(std::size_t column) const {
        return column_types_.at(column);
    }

    ~BinaryDataTableStream() {
        /* Finish writing content so that header can be written by seeking to
           the beginning of file.
        */
        write_header_();
        std::cerr << "Row count is " << get_current_row_index() << std::endl;
    }

  private:
    /* LGLSXP: sizeof(int) */
    void write_column_impl_(bool value) override {
        store_or_check_column_type({LGLSXP, sizeof(bool)});
        write(&value, sizeof(bool));
    }

    /* INTSXP: sizeof(int) */
    void write_column_impl_(int value) override {
        store_or_check_column_type({INTSXP, sizeof(int)});
        int32_t int_value = value;
        write(&int_value, sizeof(int_value));
    }

    /* INTSXP: sizeof(int) */
    void write_column_impl_(uint8_t value) override {
        store_or_check_column_type({INTSXP, sizeof(uint8_t)});
        write(&value, sizeof(value));
    }

    /* REALSXP: sizeof(double) */
    void write_column_impl_(double value) override {
        store_or_check_column_type({REALSXP, sizeof(int)});
        write(&value, sizeof(double));
    }

    /* STRSXP: variable length */
    void write_column_impl_(const std::string &value) override {
        store_or_check_column_type({STRSXP, 0});
        write_string_column_(value.c_str(), value.size());
    }

    /* STRSXP: variable length */
    void write_column_impl_(const char *value) override {
        store_or_check_column_type({STRSXP, 0});
        write_string_column_(value, strlen(value));
    }

    void write_string_column_(const char *value, uint32_t size) {
        write(&size, sizeof(size));
        write(value, size);
    }
    /* TODO - first compress the stream, then write the size in the beginning.
              Do not compress the size as well. This is very important.
              The size value is updated in the end with the correct size,
              if it is compressed, then it can't be updated. */
    void write_header_() {
        finalize();
        seek(0, SEEK_SET);
        std::uint32_t size = get_current_row_index();
        write(&size, sizeof(size));
        size = get_column_count();
        write(&size, sizeof(size));
        /* write each column name as a variable length string */
        for (auto column_index = 0; column_index < get_column_count();
             ++column_index) {
            const std::string &column_name{get_column_name(column_index)};
            const column_type_t &column_type{get_column_type(column_index)};
            size = column_name.size();
            write(&size, sizeof(size));
            write(column_name.c_str(), size);
            size = column_type.first;
            write(&size, sizeof(size));
            size = column_type.second;
            write(&size, sizeof(size));
        }
    }
    std::vector<column_type_t> column_types_;
};

#endif /* PROMISEDYNTRACER_BINARY_DATA_TABLE_STREAM_H */
