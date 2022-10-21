#include "utils.hpp"
#include <vector>
#include <string>
#include <iostream>
#include <fstream>

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
    // avformat_free_context(fmt_ctx_);
  }
};

struct Buffer {
  AVIOContext* avio_ctx_;
  std::vector<uint8_t> data_;

  Buffer(const std::vector<uint8_t>& data): data_{data} {
    // TODO: doesn't seem to be the valid usage (avio free data pointer internally?)
    avio_ctx_ = avio_alloc_context(data_.data(), data_.size(), 0, NULL, NULL, NULL, NULL);
    ASSERT(avio_ctx_);
  }

  ~Buffer() {
    avio_context_free(&avio_ctx_);
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

  // TODO: segfault on cleanup
  // $ cmake --build build/cc/Debug -- example-local
  // [2/2] Linking CXX executable example-local
  // $ ./build/cc/Debug/example-local
  // Input #0, matroska,webm, from '(null)':
  //   Metadata:
  //     encoder         : google/video-file
  //   Duration: 00:04:12.40, start: -0.007000, bitrate: N/A
  //   Stream #0:0(eng): Audio: opus, 48000 Hz, 2 channels (default)
  // Erreur de segmentation (core dumped)

  return 0;
}
