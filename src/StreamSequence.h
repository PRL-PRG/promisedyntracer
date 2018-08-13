#ifndef PROMISEDYNTRACER_STREAM_SEQUENCE_H
#define PROMISEDYNTRACER_STREAM_SEQUENCE_H

#include "BufferStream.h"
#include "FileStream.h"
#include "Stream.h"
#include "ZstdCompressionStream.h"

class StreamSequence : public Stream {

  public:
    explicit StreamSequence(const std::string &filepath,
                            std::size_t header_buffer_size,
                            int compression_level_ = 0)
        : Stream(nullptr), file_stream_{nullptr}, buffer_stream_{nullptr},
          zstd_compression_stream_{nullptr} {

        file_stream_ = new FileStream(filepath, O_WRONLY | O_CREAT);
        buffer_stream_ = new BufferStream(file_stream_);
        zstd_compression_stream_ = new ZstdCompressionStream(buffer_stream_, 1);
        set_sink(zstd_compression_stream_);

        buffer_stream_->fill(0, header_buffer_size);
        buffer_stream_->flush();
    }

    void write(const void *buffer, std::size_t bytes) override {
        get_sink()->write(buffer, bytes);
    }

    void header_segment() {
        zstd_compression_stream_->finalize();
        set_sink(buffer_stream_);
        flush();
        file_stream_->seek(0, SEEK_SET);
    }

    void flush() {
        for (Stream *stream = get_sink(); stream != nullptr;
             stream = stream->get_sink()) {
            stream->flush();
        }
    }

    ~StreamSequence() {
        flush();
        delete zstd_compression_stream_;
        delete buffer_stream_;
        delete file_stream_;
    }

  private:
    FileStream *file_stream_;
    BufferStream *buffer_stream_;
    ZstdCompressionStream *zstd_compression_stream_;
};

#endif /* PROMISEDYNTRACER_STREAM_SEQUENCE_H */
