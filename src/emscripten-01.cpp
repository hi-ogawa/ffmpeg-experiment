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
// based on example-03
//

std::vector<uint8_t> run(std::vector<uint8_t> in_data,
                         const std::string& out_format) {
  // input context
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

  // output context
  BufferOutput output_;
  AVFormatContext* ofmt_ctx_;
  avformat_alloc_output_context2(&ofmt_ctx_, NULL, out_format.c_str(), NULL);
  ASSERT(ofmt_ctx_);
  DEFER {
    avformat_free_context(ofmt_ctx_);
  };
  ofmt_ctx_->pb = output_.avio_ctx_;

  // input audio stream
  auto stream_index =
      av_find_best_stream(ifmt_ctx_, AVMEDIA_TYPE_AUDIO, -1, -1, NULL, 0);
  ASSERT(stream_index >= 0);
  AVStream* in_stream = ifmt_ctx_->streams[stream_index];
  ASSERT(in_stream);

  // output audio stream
  AVStream* out_stream = avformat_new_stream(ofmt_ctx_, nullptr);
  ASSERT(out_stream);
  ASSERT(avcodec_parameters_copy(out_stream->codecpar, in_stream->codecpar) >=
         0);
  out_stream->time_base = in_stream->time_base;

  // allocate AVPacket
  AVPacket* pkt = av_packet_alloc();
  ASSERT(pkt);
  DEFER {
    av_packet_free(&pkt);
  };

  // write header
  ASSERT(avformat_write_header(ofmt_ctx_, nullptr) >= 0);

  // copy packets
  std::vector<uint8_t> result;
  while (av_read_frame(ifmt_ctx_, pkt) >= 0) {
    ASSERT(pkt->stream_index == stream_index);
    pkt->stream_index = out_stream->index;
    av_packet_rescale_ts(pkt, in_stream->time_base, out_stream->time_base);
    ASSERT(av_interleaved_write_frame(ofmt_ctx_, pkt) == 0);
    av_packet_unref(pkt);
  }
  ASSERT(av_interleaved_write_frame(ofmt_ctx_, nullptr) == 0);

  // write trailer
  av_write_trailer(ofmt_ctx_);

  // return Vector
  return output_.output_;
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
