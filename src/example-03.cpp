// webm -> opus without transcoding (aka "-c copy")
// third_party/FFmpeg/doc/examples/muxing.c
// https://github.com/FFmpeg/FFmpeg/blob/81bc4ef14292f77b7dcea01b00e6f2ec1aea4b32/fftools/ffmpeg.c#L1782

#include <cstring>
#include <map>
#include <nlohmann/json.hpp>
#include <optional>
#include "opusenc-picture.hpp"
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

struct BufferOutput {
  AVIOContext* avio_ctx_;
  std::vector<uint8_t> output_;

  BufferOutput() {
    // ffmpeg internal buffer (needs to be allocated on our own initially)
    constexpr size_t AVIO_BUFFER_SIZE = 1 << 12;  // 4K
    auto avio_buffer = reinterpret_cast<uint8_t*>(av_malloc(AVIO_BUFFER_SIZE));
    ASSERT(avio_buffer);

    // instantiate AVIOContext
    avio_ctx_ = avio_alloc_context(avio_buffer, AVIO_BUFFER_SIZE, 1, this, NULL,
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
    output_.insert(output_.end(), buf, buf + buf_size);
    return buf_size;
  }
};

//
// AVFormatContext wrapper
//

struct FormatContext {
  AVFormatContext* ifmt_ctx_;
  AVFormatContext* ofmt_ctx_;
  BufferInput input_;
  BufferOutput output_;

  FormatContext(const std::vector<uint8_t>& input,
                const std::map<std::string, std::string>& metadata)
      : input_{input} {
    // input
    ifmt_ctx_ = avformat_alloc_context();
    ASSERT(ifmt_ctx_);
    ifmt_ctx_->pb = input_.avio_ctx_;

    // output
    avformat_alloc_output_context2(&ofmt_ctx_, NULL, "opus", NULL);
    ASSERT(ofmt_ctx_);
    ofmt_ctx_->pb = output_.avio_ctx_;

    // set metadata
    for (auto [k, v] : metadata) {
      av_dict_set(&ofmt_ctx_->metadata, k.c_str(), v.c_str(), 0);
    }
  }

  ~FormatContext() {
    ifmt_ctx_->pb = nullptr;  // feels very odd but otherwise SEGV when
                              // avformat_close_input/avio_close without
                              // avio_open is not called yet internally
    avformat_close_input(&ifmt_ctx_);
    avformat_free_context(ofmt_ctx_);
  }

  void openInput(bool debug = false) {
    ASSERT(avformat_open_input(&ifmt_ctx_, NULL, NULL, NULL) == 0);
    ASSERT(avformat_find_stream_info(ifmt_ctx_, NULL) == 0);
    if (debug) {
      av_dump_format(ifmt_ctx_, 0, NULL, 0);
    }
  }

  void runCopy() {
    // find audio stream from input
    auto stream_index =
        av_find_best_stream(ifmt_ctx_, AVMEDIA_TYPE_AUDIO, -1, -1, NULL, 0);
    ASSERT(stream_index >= 0);
    AVStream* in_stream = ifmt_ctx_->streams[stream_index];
    ASSERT(in_stream);

    // add audio stream to output and configure codec parameter
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
  }
};

//
// main
//

int main(int argc, const char** argv) {
  // parse arguments
  utils::Cli cli{argc, argv};
  auto in_file = cli.argument("--in");
  auto in_picture_file = cli.argument("--in-picture");
  auto in_metadata = cli.argument("--in-metadata");
  auto out_file = cli.argument("--out");
  if (!in_file || !out_file) {
    std::cout << cli.help() << std::endl;
    return 1;
  }

  // read data
  auto in_data = utils::readFile(in_file.value());

  // prepare metadata
  std::map<std::string, std::string> metadata;

  // simple key/value
  if (in_metadata) {
    auto data = nlohmann::json::parse(in_metadata.value());
    ASSERT(data.is_object());
    for (auto& prop : data.items()) {
      ASSERT(prop.value().is_string())
      metadata[prop.key()] = prop.value().get<std::string>();
    }
  }

  // cover art
  if (in_picture_file) {
    auto picture_data = utils::readFile(in_picture_file.value());
    metadata[opusenc_picture::TAG] = opusenc_picture::encode(picture_data);
  }

  // process
  FormatContext format_context{in_data, metadata};
  format_context.openInput(true);
  format_context.runCopy();

  // write raw audio
  utils::writeFile(out_file.value(), format_context.output_.output_);
}
