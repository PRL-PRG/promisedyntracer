#include "FileStream.h"

int open_file(const std::string &filepath, int flags, mode_t mode) {
    int fd;
    if ((fd = open(filepath.c_str(), flags, mode)) == -1) {
        error_at_line(1, errno, __FILE__, __LINE__, "unable to open '%s'",
                      filepath.c_str());
    }
    return fd;
}

void close_file(int fd, const std::string &filepath) {
    if (close(fd) == -1) {
        error_at_line(1, errno, __FILE__, __LINE__, "unable to close '%s'",
                      filepath.c_str());
    }
}

std::pair<void *, std::size_t> map_to_memory(const std::string &filepath) {

    int fd = open_file(filepath, O_RDONLY);

    struct stat file_info = {0};

    if (fstat(fd, &file_info) == -1) {
        error_at_line(1, errno, __FILE__, __LINE__,
                      "unable to get size of '%s'", filepath.c_str());
    }

    if (file_info.st_size == 0) {
        close_file(fd, filepath);
        return {NULL, 0};
    }

    void *data = mmap(NULL, file_info.st_size, PROT_READ,
                      MAP_PRIVATE | MAP_POPULATE, fd, 0);

    close_file(fd, filepath);

    if (data == MAP_FAILED) {
        error_at_line(1, errno, __FILE__, __LINE__, "failed to map '%s'",
                      filepath.c_str());
    }

    return {data, file_info.st_size};
}

void unmap_memory(void *data, std::size_t size) {
    if (munmap(data, size)) {
        error_at_line(1, errno, __FILE__, __LINE__, "failed to unmap");
    }
}
