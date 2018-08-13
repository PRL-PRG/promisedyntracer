#ifndef PROMISEDYNTRACER_TABLE_WRITER_H
#define PROMISEDYNTRACER_TABLE_WRITER_H

#include <fstream>
#include <iostream>
#include <string>
#include <vector>

class TextTableWriter {
  public:
    explicit TextTableWriter(const std::string &table_filepath,
                             size_t column_count)
        : table_filepath_{table_filepath}, column_count_{column_count},
          table_file_{table_filepath}, current_column_count_{0} {
        if (!table_file_.is_open()) {
            std::cerr << "unable to open file: " << table_filepath_
                      << std::endl;
            exit(EXIT_FAILURE);
        }
    }

    explicit TextTableWriter(const std::string &table_filepath,
                             std::vector<std::string> column_names)
        : TextTableWriter(table_filepath, column_names.size()) {
        for (size_t i = 0; i < column_names.size() - 1; ++i) {
            table_file_ << column_names[i] << get_column_separator();
        }
        table_file_ << column_names.back() << get_row_separator();
    }

    template <typename T> TextTableWriter &write_row(T field) {
        table_file_ << field;
        ++current_column_count_;
        if (current_column_count_ != get_column_count()) {
            std::cerr
                << "number of columns in the row does not match number of "
                << "headings for file " << table_filepath_ << std::endl;
        }
        current_column_count_ = 0;
        table_file_ << get_row_separator();
        return *this;
    }

    template <typename T, typename... Args>
    TextTableWriter &write_row(T field, Args... fields) {
        table_file_ << field << get_column_separator();
        ++current_column_count_;
        return write_row(fields...);
    }

    std::string get_table_filepath() const { return table_filepath_; }

    size_t get_column_count() const { return column_count_; }

    static const std::string &get_column_separator();

    static const std::string &get_row_separator();

    ~TextTableWriter() {
        if (table_file_.is_open())
            table_file_.close();
    }

  private:
    std::string table_filepath_;
    size_t column_count_;
    std::ofstream table_file_;
    size_t current_column_count_;
    static const std::string column_separator_;
    static const std::string row_separator_;
};

#endif /* PROMISEDYNTRACER_TABLE_WRITER_H */
