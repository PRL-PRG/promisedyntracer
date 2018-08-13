#ifndef PROMISEDYNTRACER_TABLE_WRITER_H
#define PROMISEDYNTRACER_TABLE_WRITER_H

#include <fstream>
#include <iostream>
#include <string>
#include <vector>

class TableWriter {
    TableWriter();

    const std::string &get_filepath() const { return table_filepath_; }

    size_t get_column_count() const { return column_count_; }

    template <typename T, typename... Args>
    TableWriter &write_row(T field, Args... fields) {}

    template <typename T> TableWriter &write_row(T field) {}

    virtual void write_column(bool value) = 0;
    virtual void write_column(int value) = 0;
    virtual void write_column(std::uint8_t value) = 0;
    virtual void write_column(double value) = 0;
    virtual void write_column(const std::string &value) = 0;
    virtual void write_column(const char *value) = 0;

    ~TableWriter();

  private:
    std::string table_filepath_;
    StreamSequence *stream_;
    std::vector<std::string> column_names_;
    std::vector<column_type_t> column_types_;
    std::size_t column_count_;
    std::size_t current_column_index_;
    std::uint32_t current_row_index_;
    static const std::size_t buffer_size_;
};

#endif /* PROMISEDYNTRACER_TABLE_WRITER_H */
