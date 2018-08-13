#include "TextTableWriter.h"

const std::string TextTableWriter::column_separator_ = " , ";

const std::string TextTableWriter::row_separator_ = "\n";

const std::string &TextTableWriter::get_column_separator() {
    return TextTableWriter::column_separator_;
}

const std::string &TextTableWriter::get_row_separator() {
    return TextTableWriter::row_separator_;
}
