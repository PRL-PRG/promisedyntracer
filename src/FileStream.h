#ifndef PROMISEDYNTRACER_FILE_STREAM_H
#define PROMISEDYNTRACER_FILE_STREAM_H

#include "Stream.h"
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <errno.h>
#include <error.h>
#include <fcntl.h>
#include <string>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <utility>
#include <zstd.h>

int open_file(const std::string &filepath, int flags, mode_t mode = 0666);
void close_file(int fd, const std::string &filepath);
std::pair<void *, std::size_t> map_to_memory(const std::string &filepath);
void unmap_memory(void *data, std::size_t size);

class FileStream : public Stream {
  public:
    FileStream(const std::string &filepath, int flags, int mode = 0666)
        : Stream(nullptr), filepath_{filepath} {
        descriptor_ = open_file(filepath, flags, mode);
    }

    void seek(off_t offset, int whence) {
        if (lseek(descriptor_, offset, whence) == static_cast<off_t>(-1)) {
            error_at_line(1, errno, __FILE__, __LINE__,
                          "unable to seek to offset %li"
                          "in %s",
                          offset, get_filepath().c_str());
        }
    }

    const std::string &get_filepath() const noexcept { return filepath_; }

    void write(const void *buffer, std::size_t bytes) override {
        using ::write;
        int written_bytes = 0;
        const char *buf = static_cast<const char *>(buffer);
        while (bytes > 0) {
            written_bytes = write(descriptor_, buf, bytes);
            if (written_bytes == -1) {
                if (errno == EINTR || errno == EAGAIN) {
                    continue;
                } else {
                    error_at_line(1, errno, __FILE__, __LINE__,
                                  "failed to write bytes to %s",
                                  get_filepath().c_str());
                }
            } else if (written_bytes == 0) {
                error_at_line(1, errno, __FILE__, __LINE__,
                              "failed to write bytes to %s",
                              get_filepath().c_str());
            } else {
                bytes -= written_bytes;
                buf += written_bytes;
            }
        }
    }

    void flush() {}

    ~FileStream() { close_file(descriptor_, get_filepath()); }

  private:
    std::string filepath_;
    int descriptor_;
};

#endif /* PROMISEDYNTRACER_FILE_STREAM_H */
