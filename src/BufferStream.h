#ifndef PROMISEDYNTRACER_BUFFER_STREAM_H
#define PROMISEDYNTRACER_BUFFER_STREAM_H

#include "Stream.h"
#include <algorithm>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>

class BufferStream : public Stream {
  public:
    explicit BufferStream(Stream *sink, std::size_t capacity = 25 * 1024 * 1024)
        : Stream(sink), capacity_{0}, index_{0}, buffer_{nullptr} {
        set_capacity(capacity);
    }

    bool is_empty() const noexcept { return index_ == 0; }

    std::size_t get_size() const noexcept { return index_; }

    std::size_t get_capacity() const noexcept { return capacity_; }

    void set_capacity(std::size_t capacity) {
        std::free(buffer_);
        if ((buffer_ = static_cast<char *>(std::calloc(capacity, 1))) ==
            nullptr) {
            std::fprintf(stderr, "unable to reserve buffer with capacity %ld",
                         capacity);
            exit(EXIT_FAILURE);
        }
        capacity_ = capacity;
    }

    void reserve(std::size_t capacity) {
        /* disallow buffer size less than 1 KB */
        capacity = capacity < 1024 ? 1024 : capacity;
        if (get_capacity() < capacity) {
            set_capacity(capacity);
        }
    }

    void fill(std::uint8_t byte, std::size_t count) {
        /* TODO - nullptr ?*/
        char *buffer =
            static_cast<char *>(std::calloc(count, sizeof(std::uint8_t)));
        for (std::size_t index = 0; index < count; ++index) {
            buffer[index] = byte;
        }
        write(buffer, count);
        std::free(buffer);
    }

    void write(const void *buffer, std::size_t bytes) override {
        const char *buf = static_cast<const char *>(buffer);
        std::size_t remaining_bytes = bytes;
        std::size_t copied_bytes = 0;
        do {
            copied_bytes =
                std::min(get_capacity() - get_size(), remaining_bytes);
            std::memcpy(buffer_ + index_, buf, copied_bytes);
            buf += copied_bytes;
            index_ += copied_bytes;
            remaining_bytes -= copied_bytes;
            if (get_size() == get_capacity())
                flush();
        } while (remaining_bytes != 0);
    }

    void flush() {
        get_sink()->write(buffer_, get_size());
        index_ = 0;
    }

    ~BufferStream() {
        flush();
        std::free(buffer_);
    }

  private:
    std::size_t capacity_;
    std::size_t index_;
    char *buffer_;
};

#endif /* PROMISEDYNTRACER_BUFFER_STREAM_H */
