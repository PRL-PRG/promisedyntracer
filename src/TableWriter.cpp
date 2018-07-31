#include "TableWriter.h"

const std::string TableWriter::column_separator_ = " , ";
const std::string TableWriter::row_separator_ = "\n";

const std::string &TableWriter::get_column_separator() {
  return TableWriter::column_separator_;
}

const std::string &TableWriter::get_row_separator() {
  return TableWriter::row_separator_;
}
