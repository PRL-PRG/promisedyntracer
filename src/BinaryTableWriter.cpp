#include "BinaryTableWriter.h"

const std::size_t BinaryTableWriter::buffer_size_ = 1024 * 1024 * 25;

std::size_t BinaryTableWriter::get_buffer_size() {
    return BinaryTableWriter::buffer_size_;
}
