#include "utils.hpp"
#include <vector>
#include <string>
#include <iostream>
#include <fstream>
#include <cstring>

// based on third_party/FFmpeg/doc/examples/avio_reading.c

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

  ~Example() {
    avformat_close_input(&fmt_ctx_);
  }
};

constexpr size_t AVIO_BUFFER_SIZE = 4096;

struct Buffer {
  AVIOContext* avio_ctx_;
  void* avio_buffer_; // ffmpeg internal buffer
  std::vector<uint8_t> data_; // actual data
  std::vector<uint8_t>::iterator data_iter_;

  Buffer(const std::vector<uint8_t>& data): data_{data} {
    data_iter_ = data_.begin();
    avio_buffer_ = av_malloc(AVIO_BUFFER_SIZE);
    ASSERT(avio_buffer_);
    avio_ctx_ = avio_alloc_context(reinterpret_cast<uint8_t*>(avio_buffer_), AVIO_BUFFER_SIZE, 0, this, Buffer::read_packet, NULL, NULL);
    ASSERT(avio_ctx_);
  }

  ~Buffer() {
    av_freep(&avio_ctx_->buffer);
    avio_context_free(&avio_ctx_);
  }

  static int read_packet(void *opaque, uint8_t *buf, int buf_size) {
    return reinterpret_cast<Buffer*>(opaque)->read_packet(buf, buf_size);
  }

  int read_packet(uint8_t *dest, int req_size) {
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

// TODO: customize callback
// void custom_log_callback(void*, int, const char*, va_list) {
// }
// av_log_set_callback(custom_log_callback);

std::vector<uint8_t> read_file(const std::string& filename) {
  std::ifstream istr(filename, std::ios::binary);
  ASSERT(istr.is_open());
  std::vector<uint8_t> data((std::istreambuf_iterator<char>(istr)), std::istreambuf_iterator<char>());
  return data;
}

int main() {
  Example example;

  // yt-dlp -f 251 -o test.webm https://www.youtube.com/watch?v=uWEcvd7wk3U
  auto data = read_file("test.webm");
  Buffer buffer(data);

  example.fmt_ctx_->pb = buffer.avio_ctx_;
  ASSERT(avformat_open_input(&example.fmt_ctx_, NULL, NULL, NULL) == 0);
  ASSERT(avformat_find_stream_info(example.fmt_ctx_, NULL) == 0);
  av_dump_format(example.fmt_ctx_, 0, NULL, 0);

  return 0;
}
