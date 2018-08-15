#ifndef PROMISEDYNTRACER_DATA_TABLE_STREAM_H
#define PROMISEDYNTRACER_DATA_TABLE_STREAM_H

#include "BufferStream.h"
#include "FileStream.h"
#include "Stream.h"
#include "ZstdCompressionStream.h"
#include "sexptypes.h"
#include <cstdint>
#include <iostream>
#include <string>
#include <vector>

class DataTableStream : public Stream {
  public:
    using column_type_t = std::pair<SEXPTYPE, uint32_t>;

    DataTableStream(const std::string &table_filepath,
                    const std::vector<std::string> &column_names, bool truncate,
                    int compression_level)
        : Stream(nullptr), table_filepath_{table_filepath},
          column_names_{column_names}, column_count_{column_names.size()},
          current_row_index_{0}, current_column_index_{0},
          file_stream_{nullptr}, buffer_stream_{nullptr},
          zstd_compression_stream_{nullptr} {

        int flags = O_WRONLY | O_CREAT;
        flags = truncate ? flags | O_TRUNC : flags;

        file_stream_ = new FileStream(table_filepath, flags);
        buffer_stream_ = new BufferStream(file_stream_);

        if (compression_level > 0) {
            zstd_compression_stream_ =
                new ZstdCompressionStream(buffer_stream_, compression_level);
            set_sink(zstd_compression_stream_);
        } else {
            set_sink(buffer_stream_);
        }
    }

    void fill(char byte, std::size_t count) {
        buffer_stream_->fill(byte, count);
    }

    void write(const void *buffer, std::size_t bytes) override {
        get_sink()->write(buffer, bytes);
    }

    void finalize() {
        if (is_compression_enabled()) {
            zstd_compression_stream_->finalize();
            set_sink(buffer_stream_);
        }
    }

    void seek(off_t offset, int whence) {
        flush();
        file_stream_->seek(offset, whence);
    }

    void flush() {
        for (Stream *stream = get_sink(); stream != nullptr;
             stream = stream->get_sink()) {
            stream->flush();
        }
    }

    int get_compression_level() const {
        return zstd_compression_stream_ == nullptr
                   ? 0
                   : zstd_compression_stream_->get_compression_level();
    }

    bool is_compression_enabled() const {
        return zstd_compression_stream_ != nullptr;
    }

    const std::string &get_filepath() const { return table_filepath_; }

    size_t get_column_count() const { return column_count_; }

    std::size_t get_current_column_index() const {
        return current_column_index_;
    }

    std::size_t get_current_row_index() const { return current_row_index_; }

    bool is_last_column() const {
        return current_column_index_ == (get_column_count() - 1);
    }

    const std::vector<std::string> &get_column_names() const {
        return column_names_;
    }

    const std::string &get_column_name(std::size_t column) const {
        return get_column_names().at(column);
    }

    template <typename T> DataTableStream *write_row(T field) {
        write_column(field);
        if (current_column_index_ != 0) {
            std::cerr
                << "number of columns in the row does not match number of "
                << "headings for file " << get_filepath() << std::endl;
        }
        return this;
    }

    template <typename T, typename... Args>
    DataTableStream *write_row(T field, Args... fields) {
        write_column(field);
        write_row(fields...);
        return this;
    }

    template <typename T> void write_column(T value) {
        write_column_impl_(value);
        ++current_column_index_;
        if (current_column_index_ == get_column_count()) {
            ++current_row_index_;
            current_column_index_ = 0;
        }
    }

    virtual ~DataTableStream() {
        flush();
        if (is_compression_enabled()) {
            delete zstd_compression_stream_;
        }
        delete buffer_stream_;
        delete file_stream_;
    }

    static std::size_t get_buffer_size();

  private:
    virtual void write_column_impl_(bool value) = 0;
    virtual void write_column_impl_(int value) = 0;
    virtual void write_column_impl_(std::uint8_t value) = 0;
    virtual void write_column_impl_(double value) = 0;
    virtual void write_column_impl_(const std::string &value) = 0;
    virtual void write_column_impl_(const char *value) = 0;

    std::string table_filepath_;
    std::vector<std::string> column_names_;
    std::size_t column_count_;

    std::size_t current_column_index_;
    std::size_t current_row_index_;

    FileStream *file_stream_;
    BufferStream *buffer_stream_;
    ZstdCompressionStream *zstd_compression_stream_;
};

#endif /* PROMISEDYNTRACER_DATA_TABLE_STREAM_H */
