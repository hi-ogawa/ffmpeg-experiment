// `av_dump_format` example based on
// third_party/FFmpeg/doc/examples/avio_reading.c

#include <cstring>
#include <string>
#include <vector>
#include "utils.hpp"

extern "C" {
#include <libavformat/avformat.h>
#include <libavformat/avio.h>
#include <libavutil/avutil.h>
}

struct Example {
  AVFormatContext* fmt_ctx_;

  Example() {
    fmt_ctx_ = avformat_alloc_context();
    ASSERT(fmt_ctx_);
  }

  ~Example() { avformat_close_input(&fmt_ctx_); }
};

//
// in memory IO
//

struct Buffer {
  AVIOContext* avio_ctx_;
  void* avio_buffer_;          // ffmpeg internal buffer
  std::vector<uint8_t> data_;  // actual data
  std::vector<uint8_t>::iterator data_iter_;

  static constexpr size_t AVIO_BUFFER_SIZE = 1 << 12;

  Buffer(const std::vector<uint8_t>& data) : data_{data} {
    data_iter_ = data_.begin();
    avio_buffer_ = av_malloc(AVIO_BUFFER_SIZE);
    ASSERT(avio_buffer_);
    avio_ctx_ = avio_alloc_context(reinterpret_cast<uint8_t*>(avio_buffer_),
                                   AVIO_BUFFER_SIZE, 0, this,
                                   Buffer::read_packet, NULL, NULL);
    ASSERT(avio_ctx_);
  }

  ~Buffer() {
    av_freep(&avio_ctx_->buffer);
    avio_context_free(&avio_ctx_);
  }

  static int read_packet(void* opaque, uint8_t* buf, int buf_size) {
    return reinterpret_cast<Buffer*>(opaque)->read_packet(buf, buf_size);
  }

  int read_packet(uint8_t* dest, int req_size) {
    int remaining = data_.end() - data_iter_;
    int read_size = std::min(req_size, remaining);
    if (read_size == 0) {
      return AVERROR_EOF;
    }
    std::memcpy(dest, &*data_iter_, read_size);
    data_iter_ += read_size;
    return read_size;
  }
};

//
// custom log callback
//

struct Logger;
Logger* g_logger = nullptr;

struct Logger {
  std::vector<std::tuple<int, std::string>> logs_;
  bool debug_ = false;

  Logger() {
    ASSERT(!g_logger);
    g_logger = this;
    av_log_set_callback(Logger::log_callback_main);
  }

  ~Logger() {
    g_logger = nullptr;
    av_log_set_callback(av_log_default_callback);
  }

  static void log_callback_main(void*, int level, const char* fmt, va_list vl) {
    g_logger->log_callback(level, fmt, vl);
  }

  void log_callback(int level, const char* fmt, va_list vl) {
    auto log = format_log(fmt, vl);
    if (debug_) {
      std::cout << "[" << level << "] " << log << std::flush;
    }
    logs_.push_back(std::make_tuple(level, log));
  }

  static std::string format_log(const char* fmt, va_list vl) {
    // cf. https://stackoverflow.com/a/49812356

    // "dry run" to compute the length
    va_list vl_copy;
    va_copy(vl_copy, vl);
    auto len = std::vsnprintf(NULL, 0, fmt, vl_copy);
    va_end(vl_copy);

    // print to buffer
    ASSERT(len >= 0);
    std::vector<char> buffer(len + 1);
    std::vsnprintf(buffer.data(), buffer.size(), fmt, vl);
    va_end(vl);

    return std::string{buffer.data(), buffer.size()};
  }
};

//
// main
//

int main(int argc, const char** argv) {
  Logger logger;
  logger.debug_ = true;

  utils::Cli cli{argc, argv};
  auto infile = cli.argument<std::string>("--in").value_or("test.webm");

  auto data = utils::readFile(infile);
  Buffer buffer(data);

  Example example;
  example.fmt_ctx_->pb = buffer.avio_ctx_;
  ASSERT(avformat_open_input(&example.fmt_ctx_, NULL, NULL, NULL) == 0);
  ASSERT(avformat_find_stream_info(example.fmt_ctx_, NULL) == 0);
  av_dump_format(example.fmt_ctx_, 0, NULL, 0);

  return 0;
}
