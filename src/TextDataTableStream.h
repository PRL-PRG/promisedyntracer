#ifndef PROMISEDYNTRACER_TEXT_DATA_TABLE_STREAM_H
#define PROMISEDYNTRACER_TEXT_DATA_TABLE_STREAM_H

#include "DataTableStream.h"
#include <fstream>
#include <iostream>
#include <string>
#include <vector>

class TextDataTableStream : public DataTableStream {
  public:
    explicit TextDataTableStream(const std::string &table_filepath,
                                 const std::vector<std::string> &column_names,
                                 bool truncate, int compression_level)
        : DataTableStream(table_filepath, column_names, truncate,
                          compression_level) {

        contents_.reserve(1024);

        const char *separator = get_column_separator().c_str();
        std::size_t bytes = get_column_separator().size();

        if (get_column_count() == 0) {
            return;
        }

        for (size_t i = 0; i < get_column_count() - 1; ++i) {
            contents_.append(column_names[i]);
            contents_.append(get_column_separator());
        }
        contents_.append(column_names.back());
        contents_.append(get_row_separator());
        write(contents_.c_str(), contents_.size());
        contents_.clear();
    }

    ~TextDataTableStream() { finalize(); }

    static const std::string &get_column_separator();

    static const std::string &get_row_separator();

  private:
    void write_column_impl_(bool value) override {
        contents_.append(value ? "1" : "0");
        flush_contents_();
    }

    void write_column_impl_(int value) override { write_column_(value); }

    void write_column_impl_(std::uint8_t value) override {
        write_column_(value);
    }

    void write_column_impl_(double value) override { write_column_(value); }

    void write_column_impl_(const std::string &value) override {
        contents_.append(value);
        flush_contents_();
    }

    void write_column_impl_(const char *value) override {
        contents_.append(value);
        flush_contents_();
    }

    template <typename T> void write_column_(T value) {
        contents_.append(std::to_string(value));
        flush_contents_();
    }

    void flush_contents_() {
        contents_.append(get_current_separator_());
        write(contents_.c_str(), contents_.size());
        contents_.clear();
    }

    const std::string &get_current_separator_() const {
        return is_last_column() ? get_row_separator() : get_column_separator();
    }

    std::string contents_;
    static const std::string column_separator_;
    static const std::string row_separator_;
};

#endif /* PROMISEDYNTRACER_TEXT_DATA_TABLE_STREAM_H */
