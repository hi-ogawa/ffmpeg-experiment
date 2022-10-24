#include <cstring>
#include <optional>
#include "utils-ffmpeg.hpp"
#include "utils.hpp"

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavformat/avio.h>
#include <libavutil/avutil.h>
}

//
// based on example-04
//

std::string run(std::vector<uint8_t> in_data) {
  //
  // input
  //
  BufferInput input_{in_data};
  AVFormatContext* ifmt_ctx_ = avformat_alloc_context();
  ASSERT(ifmt_ctx_);
  DEFER {
    avformat_close_input(&ifmt_ctx_);
  };
  ifmt_ctx_->pb = input_.avio_ctx_;
  ifmt_ctx_->flags |= AVFMT_FLAG_CUSTOM_IO;

  ASSERT(avformat_open_input(&ifmt_ctx_, NULL, NULL, NULL) == 0);
  ASSERT(avformat_find_stream_info(ifmt_ctx_, NULL) == 0);

  std::ostringstream ostr;
  ostr << utils::mapFromAVDictionary(ifmt_ctx_->metadata);
  return ostr.str();
}

//
// embind
//

#include <emscripten.h>
#include <emscripten/bind.h>
#include <emscripten/val.h>

using namespace emscripten;

template <typename T>
val Vector_view(const std::vector<T>& self) {
  return val(typed_memory_view(self.size(), self.data()));
}

EMSCRIPTEN_BINDINGS(emscripten_00) {
  register_vector<uint8_t>("Vector").function("view", &Vector_view<uint8_t>);

  // "run" is reserved for emscripten exports
  function("runTest", &run);
}
