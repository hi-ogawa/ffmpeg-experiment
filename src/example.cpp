#include "utils.hpp"

extern "C" {
  #include <libavformat/avformat.h>
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

int main() {
  Example example;
  return 0;
}
