#include "TextDataTableStream.h"

const std::string TextDataTableStream::column_separator_ = " , ";

const std::string TextDataTableStream::row_separator_ = "\n";

const std::string &TextDataTableStream::get_column_separator() {
    return TextDataTableStream::column_separator_;
}

const std::string &TextDataTableStream::get_row_separator() {
    return TextDataTableStream::row_separator_;
}
