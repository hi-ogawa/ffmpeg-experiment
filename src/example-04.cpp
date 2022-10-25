// transcode audio stream (webm -> opus)
// third_party/FFmpeg/doc/examples/transcoding.c

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

template <class Fn>
void decodePacket(AVCodecContext* dec_ctx,
                  const AVPacket* src_pkt,
                  AVFrame* dst_frame,
                  Fn on_frame_callback) {
  ASSERT(avcodec_send_packet(dec_ctx, src_pkt) >= 0);
  while (true) {
    auto ret = avcodec_receive_frame(dec_ctx, dst_frame);
    if (ret == AVERROR_EOF || ret == AVERROR(EAGAIN)) {
      return;
    }
    ASSERT(ret >= 0);
    on_frame_callback();
    av_frame_unref(dst_frame);
  }
}

template <class Fn>
void encodeFrame(AVCodecContext* enc_ctx,
                 const AVFrame* src_frame,
                 AVPacket* dst_pkt,
                 Fn on_packet_callback) {
  // fails
  ASSERT_AV(avcodec_send_frame(enc_ctx, src_frame));
  while (true) {
    auto ret = avcodec_receive_packet(enc_ctx, dst_pkt);
    if (ret == AVERROR_EOF || ret == AVERROR(EAGAIN)) {
      return;
    }
    ASSERT(ret >= 0);
    on_packet_callback();
    av_packet_unref(dst_pkt);
  }
}

struct FormatContext {
  AVFormatContext* ifmt_ctx_;
  AVFormatContext* ofmt_ctx_;
  BufferInput input_;
  BufferOutput output_;

  FormatContext(const std::vector<uint8_t>& input,
                const std::string& output_format)
      : input_{input} {
    // input
    ifmt_ctx_ = avformat_alloc_context();
    ASSERT(ifmt_ctx_);
    ifmt_ctx_->pb = input_.avio_ctx_;
    ifmt_ctx_->flags |= AVFMT_FLAG_CUSTOM_IO;

    // output
    avformat_alloc_output_context2(&ofmt_ctx_, NULL, output_format.c_str(),
                                   NULL);
    ASSERT(ofmt_ctx_);
    ofmt_ctx_->pb = output_.avio_ctx_;
    ofmt_ctx_->flags |= AVFMT_FLAG_CUSTOM_IO;
  }

  ~FormatContext() {
    avformat_close_input(&ifmt_ctx_);
    avformat_free_context(ofmt_ctx_);
  }

  void transcode() {
    // initialize input
    ASSERT(avformat_open_input(&ifmt_ctx_, NULL, NULL, NULL) == 0);
    ASSERT(avformat_find_stream_info(ifmt_ctx_, NULL) == 0);
    av_dump_format(ifmt_ctx_, 0, NULL, 0);
    dbg(utils::mapFromAVDictionary(ifmt_ctx_->metadata));

    // find input audio stream
    auto stream_index =
        av_find_best_stream(ifmt_ctx_, AVMEDIA_TYPE_AUDIO, -1, -1, NULL, 0);
    ASSERT(stream_index >= 0);
    AVStream* in_stream = ifmt_ctx_->streams[stream_index];
    ASSERT(in_stream);

    // instantiate decoder
    const AVCodec* dec = avcodec_find_decoder(in_stream->codecpar->codec_id);
    ASSERT(dec);
    AVCodecContext* dec_ctx = avcodec_alloc_context3(dec);
    ASSERT(dec_ctx);
    DEFER {
      avcodec_free_context(&dec_ctx);
    };
    ASSERT(avcodec_open2(dec_ctx, dec, NULL) == 0);
    ASSERT(avcodec_parameters_to_context(dec_ctx, in_stream->codecpar) >= 0);

    // create and configure output audio stream
    AVStream* out_stream = avformat_new_stream(ofmt_ctx_, nullptr);
    ASSERT(out_stream);

    // instantiate encoder
    const AVCodec* enc = avcodec_find_encoder(dec->id);
    ASSERT(enc);
    AVCodecContext* enc_ctx = avcodec_alloc_context3(enc);
    ASSERT(enc_ctx);
    DEFER {
      avcodec_free_context(&enc_ctx);
    };
    enc_ctx->sample_rate = dec_ctx->sample_rate;
    ASSERT(av_channel_layout_copy(&enc_ctx->ch_layout, &dec_ctx->ch_layout) ==
           0);
    enc_ctx->sample_fmt = dec_ctx->sample_fmt;
    enc_ctx->time_base = {1, enc_ctx->sample_rate};
    enc_ctx->extradata = dec_ctx->extradata;  // TODO: might SEGV
    enc_ctx->extradata_size = dec_ctx->extradata_size;
    enc_ctx->strict_std_compliance = -2;  // opus encoder is experimental (note
                                          // that this is not external libopus)
    ASSERT(avcodec_open2(enc_ctx, enc, NULL) == 0);
    ASSERT(avcodec_parameters_from_context(out_stream->codecpar, enc_ctx) >= 0);
    av_dump_format(ofmt_ctx_, 0, nullptr, 1);

    // allocate AVFrame and AVPacket
    AVFrame* in_frame = av_frame_alloc();
    ASSERT(in_frame);
    DEFER {
      av_frame_free(&in_frame);
    };
    AVPacket* in_pkt = av_packet_alloc();
    ASSERT(in_pkt);
    DEFER {
      av_packet_free(&in_pkt);
    };
    AVPacket* out_pkt = av_packet_alloc();
    ASSERT(out_pkt);
    DEFER {
      av_packet_free(&out_pkt);
    };

    // write header
    // TODO: ogg/opus requires extradata to be configured?
    // https://github.com/FFmpeg/FFmpeg/blob/81bc4ef14292f77b7dcea01b00e6f2ec1aea4b32/libavformat/oggenc.c#L502-L507
    ASSERT(avformat_write_header(ofmt_ctx_, nullptr) >= 0);

    // transcode packets
    while (av_read_frame(ifmt_ctx_, in_pkt) >= 0) {
      if (in_pkt->stream_index == in_stream->index) {
        decodePacket(dec_ctx, in_pkt, in_frame, [&]() {
          // TODO: encode and write frame
          dbg(in_frame->pts);
          encodeFrame(enc_ctx, in_frame, out_pkt, [&]() {
            dbg(out_pkt->pts);
            ASSERT(av_interleaved_write_frame(ofmt_ctx_, out_pkt) >= 0);
          });
        });
      }
      av_packet_unref(in_pkt);
    }
    encodeFrame(enc_ctx, nullptr, out_pkt, [&]() {
      ASSERT(av_interleaved_write_frame(ofmt_ctx_, out_pkt) >= 0);
    });
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
  auto in_file = cli.argument<std::string>("--in");
  auto out_file = cli.argument<std::string>("--out");
  if (!in_file || !out_file) {
    std::cout << cli.help() << std::endl;
    return 1;
  }

  // read data
  auto in_data = utils::readFile(in_file.value());

  // transcode
  FormatContext format_context{in_data, "ogg"};
  format_context.transcode();

  // write data
  utils::writeFile(out_file.value(), format_context.output_.output_);
}
