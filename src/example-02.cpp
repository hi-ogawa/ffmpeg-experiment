// third_party/FFmpeg/doc/examples/demuxing_decoding.c
// third_party/FFmpeg/doc/examples/transcoding.c
// third_party/FFmpeg/doc/examples/muxing.c

#include <cstring>
#include "utils.hpp"

extern "C" {
#include <libavformat/avformat.h>
#include <libavformat/avio.h>
#include <libavutil/avutil.h>
}

//
// AVIOContext wrapper for in-memory data
//

struct BufferInput {
  AVIOContext* avio_ctx_;
  std::vector<uint8_t> input_;
  size_t input_pos_ = 0;

  BufferInput(const std::vector<uint8_t>& input) : input_{input} {
    // ffmpeg internal buffer (needs to be allocated on our own initially)
    constexpr size_t AVIO_BUFFER_SIZE = 1 << 12;  // 4K
    auto avio_buffer = reinterpret_cast<uint8_t*>(av_malloc(AVIO_BUFFER_SIZE));
    ASSERT(avio_buffer);

    // instantiate AVIOContext (`seek` doesn't seem necessary but why not)
    avio_ctx_ =
        avio_alloc_context(avio_buffer, AVIO_BUFFER_SIZE, 0, this,
                           BufferInput::readPacket, NULL, BufferInput::seek);
    ASSERT(avio_ctx_);
  }

  ~BufferInput() {
    av_freep(&avio_ctx_->buffer);
    avio_context_free(&avio_ctx_);
  }

  static int readPacket(void* opaque, uint8_t* buf, int buf_size) {
    return reinterpret_cast<BufferInput*>(opaque)->readPacketImpl(buf,
                                                                  buf_size);
  }

  int readPacketImpl(uint8_t* buf, int buf_size) {
    int read_size = std::min<int>(buf_size, input_.size() - input_pos_);
    if (read_size == 0) {
      return AVERROR_EOF;
    }
    std::memcpy(buf, &input_[input_pos_], read_size);
    input_pos_ += read_size;
    return read_size;
  }

  static int64_t seek(void* opaque, int64_t offset, int whence) {
    return reinterpret_cast<BufferInput*>(opaque)->seekImpl(offset, whence);
  }

  int64_t seekImpl(int64_t offset, int whence) {
    // cf. io_seek in third_party/FFmpeg/tools/target_dem_fuzzer.c

    if (whence == AVSEEK_SIZE) {
      return input_.size();
    }

    if (whence == SEEK_CUR) {
      offset += input_pos_;
    } else if (whence == SEEK_END) {
      offset = input_.size() - offset;
    }

    if (offset < 0 || input_.size() < (size_t)offset) {
      return -1;
    }
    input_pos_ = (size_t)offset;
    return 0;
  }
};

struct BufferOutput {
  AVIOContext* avio_ctx_;
  std::vector<uint8_t> output_;

  BufferOutput() {
    // ffmpeg internal buffer (needs to be allocated on our own initially)
    constexpr size_t AVIO_BUFFER_SIZE = 1 << 12;  // 4K
    auto avio_buffer = reinterpret_cast<uint8_t*>(av_malloc(AVIO_BUFFER_SIZE));
    ASSERT(avio_buffer);

    // instantiate AVIOContext
    avio_ctx_ = avio_alloc_context(avio_buffer, AVIO_BUFFER_SIZE, 0, this, NULL,
                                   BufferOutput::writePacket, NULL);
    ASSERT(avio_ctx_);
  }

  ~BufferOutput() {
    av_freep(&avio_ctx_->buffer);
    avio_context_free(&avio_ctx_);
  }

  static int writePacket(void* opaque, uint8_t* buf, int buf_size) {
    return reinterpret_cast<BufferOutput*>(opaque)->writePacketImpl(buf,
                                                                    buf_size);
  }

  int writePacketImpl(uint8_t* buf, int buf_size) {
    auto size = output_.size();
    output_.resize(size + buf_size);
    std::memcpy(&output_[size], buf, buf_size);
    return buf_size;
  }
};

//
// AVFormatContext wrapper
//

struct FormatContext {
  AVFormatContext* ifmt_ctx_;
  AVFormatContext* ofmt_ctx_;  // TODO
  BufferInput& bytes_io_;

  FormatContext(BufferInput& bytes_io) : bytes_io_{bytes_io} {
    ifmt_ctx_ = avformat_alloc_context();
    ASSERT(ifmt_ctx_);
    ifmt_ctx_->pb = bytes_io_.avio_ctx_;
  }

  ~FormatContext() {
    ifmt_ctx_->pb = nullptr;  // feels very odd but otherwise SEGV when
                              // avformat_close_input/avio_close without
                              // avio_open is not called yet internally
    avformat_close_input(&ifmt_ctx_);
  }

  void openInput(bool debug = false) {
    ASSERT(avformat_open_input(&ifmt_ctx_, NULL, NULL, NULL) == 0);
    ASSERT(avformat_find_stream_info(ifmt_ctx_, NULL) == 0);
    if (debug) {
      av_dump_format(ifmt_ctx_, 0, NULL, 0);
    }
  }

  std::vector<uint8_t> decodeAudio() {
    auto stream_index =
        av_find_best_stream(ifmt_ctx_, AVMEDIA_TYPE_AUDIO, -1, -1, NULL, 0);
    ASSERT(stream_index >= 0);
    AVStream* stream = ifmt_ctx_->streams[stream_index];
    // dbg(streamm)

    // avcodec_find_decoder
    // stream->codecpar->codec_id;

    AVFrame* frame = av_frame_alloc();
    ASSERT(frame);
    DEFER {
      av_frame_free(&frame);
    };

    AVPacket* pkt = av_packet_alloc();
    ASSERT(pkt);
    DEFER {
      av_packet_free(&pkt);
    };

    std::vector<uint8_t> result;
    while (av_read_frame(ifmt_ctx_, pkt) >= 0) {
      // if (pkt->stream_index == stream_index) {
      // }
      dbg(pkt->stream_index, pkt->pts, pkt->size);
      av_packet_unref(pkt);
    }

    return result;
  }
};

//
// main
//

int main(int argc, const char** argv) {
  // parse arguments
  utils::Cli cli{argc, argv};
  auto in_file = cli.argument<std::string>("--in");
  auto out_file = cli.argument<std::string>("--out");
  if (!in_file || !out_file) {
    std::cout << cli.help() << std::endl;
    return 1;
  }

  // avio
  auto data = utils::read_file(in_file.value());
  BufferInput bytes_io{data};

  // avformat
  FormatContext format_context(bytes_io);
  format_context.openInput(true);
  format_context.decodeAudio();
}
