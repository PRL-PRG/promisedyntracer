#ifndef PROMISEDYNTRACER_BINARY_TABLE_WRITER_H
#define PROMISEDYNTRACER_BINARY_TABLE_WRITER_H

/* - get_column_name  and get_column_names
   - get_column_type (initialize column_type to NILSXP */

#include "StreamSequence.h"
#include "sexptypes.h"
#include <string>
#include <vector>

class BinaryTableWriter {
  public:
    using column_type_t = std::pair<SEXPTYPE, uint32_t>;

    explicit BinaryTableWriter(const std::string &table_filepath,
                               const std::vector<std::string> &column_names)
        : table_filepath_{table_filepath}, column_names_{column_names},
          column_types_{column_names.size(), {NILSXP, 0}},
          column_count_{column_names.size()}, current_column_index_{0},
          current_row_index_{0} {

        std::size_t header_buffer_size = 8;

        for (const auto &column_name : column_names) {
            header_buffer_size += 4 + column_name.size() + 4 + 4;
        }
        stream_ = new StreamSequence(table_filepath, header_buffer_size, 1);
    }

    size_t get_column_count() const { return column_count_; }

    const std::vector<std::string> &get_column_names() const {
        return column_names_;
    }

    const std::string &get_column_name(std::size_t column) const {
        return get_column_names().at(column);
    }

    const column_type_t &get_column_type(std::size_t column) const {
        return column_types_.at(column);
    }

    const std::string &get_filepath() const { return table_filepath_; }

    template <typename T, typename... Args>
    BinaryTableWriter &write_row(T field, Args... fields) {
        write_column(field);
        return write_row(fields...);
    }

    template <typename T> BinaryTableWriter &write_row(T field) {
        write_column(field);
        if (current_column_index_ != get_column_count()) {
            std::cerr
                << "number of columns in the row does not match number of "
                << "headings for file " << get_filepath() << std::endl;
        }
        current_column_index_ = 0;
        ++current_row_index_;
        return *this;
    }

    void store_or_check_column_type(const column_type_t &column_type) {
        if (current_row_index_ == 0) {
            column_types_[current_column_index_] = column_type;
        } else if (column_types_[current_column_index_] != column_type) {
            std::fprintf(
                stderr,
                "column type mismatch: expected %s of %d bytes at column "
                "%lo of file %s",
                sexptype_to_string(column_type.first).c_str(),
                column_type.second, current_column_index_,
                get_filepath().c_str());
            exit(EXIT_FAILURE);
        }
    }

    /* LGLSXP: sizeof(int) */
    BinaryTableWriter &write_column(bool value) {
        store_or_check_column_type({INTSXP, sizeof(bool)});
        uint8_t bool_value = value;
        stream_->write(&bool_value, sizeof(bool_value));
        ++current_column_index_;
        return *this;
    }

    /* INTSXP: sizeof(int) */
    BinaryTableWriter &write_column(int value) {
        store_or_check_column_type({INTSXP, sizeof(int)});
        int32_t int_value = value;
        stream_->write(&int_value, sizeof(int_value));
        ++current_column_index_;
        return *this;
    }

    /* INTSXP: sizeof(int) */
    BinaryTableWriter &write_column(uint8_t value) {
        store_or_check_column_type({INTSXP, sizeof(uint8_t)});
        stream_->write(&value, sizeof(value));
        ++current_column_index_;
        return *this;
    }

    /* REALSXP: sizeof(double) */
    BinaryTableWriter &write_column(double value) {
        store_or_check_column_type({REALSXP, sizeof(int)});
        stream_->write(&value, sizeof(double));
        ++current_column_index_;
        return *this;
    }

    /* STRSXP: variable length */
    BinaryTableWriter &write_column(std::string value) {
        store_or_check_column_type({STRSXP, 0});
        return write_string_column(value.c_str(), value.size());
    }

    /* STRSXP: variable length */
    BinaryTableWriter &write_column(const char *value) {
        store_or_check_column_type({STRSXP, 0});
        return write_string_column(value, strlen(value));
    }

    BinaryTableWriter &write_string_column(const char *value, uint32_t size) {
        stream_->write(&size, sizeof(size));
        stream_->write(value, size);
        ++current_column_index_;
        return *this;
    }

    ~BinaryTableWriter() {
        /* Finish writing content so that header can be written by seeking to
           the beginning of file.
        */
        write_header_();
        std::cerr << "Row count is " << current_row_index_ << std::endl;
        delete stream_;
    }

    static std::size_t get_buffer_size();

  private:
    /* TODO - first compress the stream, then write the size in the beginning.
              Do not compress the size as well. This is very important.
              The size value is updated in the end with the correct size,
              if it is compressed, then it can't be updated. */
    void write_header_() {
        stream_->header_segment();
        uint32_t size = current_row_index_;
        stream_->write(&size, sizeof(size));
        size = get_column_count();
        stream_->write(&size, sizeof(size));
        /* write each column name as a variable length string */
        for (auto column_index = 0; column_index < get_column_count();
             ++column_index) {
            const std::string &column_name{get_column_name(column_index)};
            const column_type_t &column_type{get_column_type(column_index)};
            size = column_name.size();
            stream_->write(&size, sizeof(size));
            stream_->write(column_name.c_str(), size);
            size = column_type.first;
            stream_->write(&size, sizeof(size));
            size = column_type.second;
            stream_->write(&size, sizeof(size));
        }
    }

    std::string table_filepath_;
    StreamSequence *stream_;
    std::vector<std::string> column_names_;
    std::vector<column_type_t> column_types_;
    std::size_t column_count_;
    std::size_t current_column_index_;
    std::uint32_t current_row_index_;
    static const std::size_t buffer_size_;
};

#endif /* PROMISEDYNTRACER_BINARY_TABLE_WRITER_H */
