// demux/decode example based on
// third_party/FFmpeg/doc/examples/demuxing_decoding.c

#include <cstring>
#include "utils.hpp"

extern "C" {
#include <libavcodec/avcodec.h>
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

//
// AVFormatContext wrapper
//

struct FormatContext {
  AVFormatContext* ifmt_ctx_;
  BufferInput& input_;

  FormatContext(BufferInput& bytes_io) : input_{bytes_io} {
    ifmt_ctx_ = avformat_alloc_context();
    ASSERT(ifmt_ctx_);
    ifmt_ctx_->pb = input_.avio_ctx_;
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
    // find audio stream
    auto stream_index =
        av_find_best_stream(ifmt_ctx_, AVMEDIA_TYPE_AUDIO, -1, -1, NULL, 0);
    ASSERT(stream_index >= 0);
    AVStream* stream = ifmt_ctx_->streams[stream_index];

    // find decoder and instantiate AVCodecContext
    const AVCodec* dec = avcodec_find_decoder(stream->codecpar->codec_id);
    ASSERT(dec);
    AVCodecContext* dec_ctx = avcodec_alloc_context3(dec);
    ASSERT(dec_ctx);
    DEFER {
      avcodec_free_context(&dec_ctx);
    };
    ASSERT(avcodec_parameters_to_context(dec_ctx, stream->codecpar) >= 0);
    avcodec_open2(dec_ctx, dec, NULL);

    // allocate AVFrame and AVPacket
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

    // read and decode packets
    std::vector<uint8_t> result;
    while (av_read_frame(ifmt_ctx_, pkt) >= 0) {
      // dbg(pkt->pts, pkt->dts, pkt->duration);
      if (pkt->stream_index == stream_index) {
        decodePacket(dec_ctx, pkt, frame, result);
      }
      av_packet_unref(pkt);
    }
    decodePacket(dec_ctx, nullptr, frame, result);
    return result;
  }

  static void decodePacket(AVCodecContext* dec_ctx,
                           const AVPacket* pkt,
                           AVFrame* frame,
                           std::vector<uint8_t>& dest) {
    ASSERT(avcodec_send_packet(dec_ctx, pkt) >= 0);
    while (true) {
      auto ret = avcodec_receive_frame(dec_ctx, frame);
      if (ret == AVERROR_EOF || ret == AVERROR(EAGAIN)) {
        return;
      }
      ASSERT(ret >= 0);
      outputFrame(frame, dest);
      av_frame_unref(frame);
    }
  }

  static void outputFrame(AVFrame* frame, std::vector<uint8_t>& dest) {
    auto format = (enum AVSampleFormat)(frame->format);
    auto fmt_name = av_get_sample_fmt_name(format);
    ASSERT(fmt_name);
    // e.g. (fltp, 2) = (floating point planer, 2 channels)
    // dbg(fmt_name, frame->ch_layout.nb_channels);
    size_t size = frame->nb_samples * av_get_bytes_per_sample(format);
    uint8_t* src = frame->extended_data[0];
    dest.insert(dest.end(), src, src + size);
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
  auto data = utils::readFile(in_file.value());
  BufferInput bytes_io{data};

  // avformat
  FormatContext format_context(bytes_io);
  format_context.openInput(true);
  auto decoded = format_context.decodeAudio();

  // write raw audio
  utils::writeFile(out_file.value(), decoded);
}
