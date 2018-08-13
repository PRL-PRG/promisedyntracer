#ifndef PROMISEDYNTRACER_ZSTD_COMPRESSION_STREAM_H
#define PROMISEDYNTRACER_ZSTD_COMPRESSION_STREAM_H

#include "Stream.h"
#include <zstd.h>

class ZstdCompressionStream : public Stream {
  public:
    ZstdCompressionStream(Stream *sink, int compression_level)
        : Stream(sink), compression_level_{compression_level},
          input_buffer_{nullptr}, input_buffer_size_{0}, input_buffer_index_{0},
          output_buffer_{nullptr}, output_buffer_size_{0},
          compression_stream_{nullptr} {

        input_buffer_size_ = ZSTD_CStreamInSize();
        input_buffer_ = static_cast<char *>(
            std::malloc(input_buffer_size_)); /* TODO - nullptr? */

        output_buffer_size_ = ZSTD_CStreamOutSize();
        output_buffer_ = static_cast<char *>(
            std::malloc(output_buffer_size_)); /* TODO - nullptr? */

        compression_stream_ = ZSTD_createCStream();
        if (compression_stream_ == NULL) {
            fprintf(stderr, "ZSTD_createCStream() error \n");
            exit(EXIT_FAILURE);
        }

        const size_t init_result =
            ZSTD_initCStream(compression_stream_, compression_level_);

        if (ZSTD_isError(init_result)) {
            fprintf(stderr, "ZSTD_initCStream() error : %s \n",
                    ZSTD_getErrorName(init_result));
            exit(EXIT_FAILURE);
        }
    }

    void write(const void *buffer, std::size_t bytes) override {
        const char *buf = static_cast<const char *>(buffer);
        std::size_t copied_bytes = 0;
        while (bytes != 0) {
            copied_bytes =
                std::min(input_buffer_size_ - input_buffer_index_, bytes);
            std::memcpy(input_buffer_ + input_buffer_index_, buf, copied_bytes);
            buf += copied_bytes;
            input_buffer_index_ += copied_bytes;
            bytes -= copied_bytes;
            if (input_buffer_index_ == input_buffer_size_)
                flush();
        }
    }

    void flush() {
        ZSTD_inBuffer input{input_buffer_, input_buffer_index_, 0};
        ZSTD_outBuffer output{output_buffer_, output_buffer_size_, 0};
        std::size_t compressed_bytes = 0;
        while (input.pos < input.size) {
            output.pos = 0;
            /* compressed_bytes is guaranteed to be <= ZSTD_CStreamInSize() */
            compressed_bytes =
                ZSTD_compressStream(compression_stream_, &output, &input);

            if (ZSTD_isError(compressed_bytes)) {
                fprintf(stderr, "ZSTD_compressStream() error : %s \n",
                        ZSTD_getErrorName(compressed_bytes));
                exit(EXIT_FAILURE);
            }

            /* Safely handle case when `buffInSize` is manually changed to a
               value < ZSTD_CStreamInSize()*/
            if (compressed_bytes > input.size)
                compressed_bytes = input.size;

            get_sink()->write(output.dst, output.pos);
        }

        input_buffer_index_ = 0;
    }

    void finalize() {
        flush();
        if (output_buffer_ != nullptr && input_buffer_ != nullptr) {
            flush();
            size_t unflushed;
            ZSTD_outBuffer output = {output_buffer_, output_buffer_size_, 0};
            while (unflushed = ZSTD_endStream(compression_stream_, &output)) {
                /* close frame */
                if (ZSTD_isError(unflushed)) {
                    fprintf(stderr, "ZSTD_endStream() error : %s \n",
                            ZSTD_getErrorName(unflushed));
                    exit(EXIT_FAILURE);
                }
                get_sink()->write(output.dst, output.pos);
                output.pos = 0;
            }

            ZSTD_freeCStream(compression_stream_);
            std::free(input_buffer_);
            input_buffer_ = nullptr;
            std::free(output_buffer_);
            output_buffer_ = nullptr;
        }
    }

    ~ZstdCompressionStream() { finalize(); }

  private:
    int compression_level_;
    char *input_buffer_;
    std::size_t input_buffer_size_;
    std::size_t input_buffer_index_;
    char *output_buffer_;
    std::size_t output_buffer_size_;
    ZSTD_CStream *compression_stream_;
};

#endif /* PROMISEDYNTRACER_ZSTD_COMPRESSION_STREAM_H */
