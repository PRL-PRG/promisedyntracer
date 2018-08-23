#ifndef PROMISEDYNTRACER_STREAM_H
#define PROMISEDYNTRACER_STREAM_H

#include <cstddef>

class Stream {
  public:
    explicit Stream(Stream *sink) { set_sink(sink); }
    virtual Stream *get_sink() { return sink_; }
    virtual void set_sink(Stream *sink) { sink_ = sink; }
    virtual void write(const void *buffer, std::size_t bytes) = 0;
    virtual void flush() = 0;

  private:
    Stream *sink_;
};

#endif /* PROMISEDYNTRACER_STREAM_H */
