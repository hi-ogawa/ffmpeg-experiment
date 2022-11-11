// convert from webm to opus while copying selected timestamp range

#include <cstring>
#include <nlohmann/json.hpp>
#include <optional>
#include "utils-ffmpeg.hpp"
#include "utils.hpp"

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavformat/avio.h>
#include <libavutil/avutil.h>
}

std::vector<uint8_t> convert(const std::vector<uint8_t>& in_data,
                             const std::string& out_format,
                             double start_time,  // -1 to indicate no value
                             double end_time) {
  // validate timestamp
  if (start_time >= 0 && end_time >= 0) {
    ASSERT(start_time <= end_time);
  }

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

  // convert to "time base" unit
  int64_t start_time_tb =
      start_time >= 0
          ? av_rescale_q(static_cast<int64_t>(start_time * AV_TIME_BASE),
                         AV_TIME_BASE_Q, in_stream->time_base)
          : -1;
  int64_t end_time_tb =
      end_time >= 0
          ? av_rescale_q(static_cast<int64_t>(end_time * AV_TIME_BASE),
                         AV_TIME_BASE_Q, in_stream->time_base)
          : -1;

  // copy packets with timestamp filtering
  std::vector<uint8_t> result;
  while (av_read_frame(ifmt_ctx_, pkt) >= 0) {
    DEFER {
      av_packet_unref(pkt);
    };
    if (end_time >= 0) {
      if (pkt->pts > end_time_tb) {
        break;
      }
    }
    if (start_time >= 0) {
      if (pkt->pts < start_time_tb) {
        continue;
      }
      pkt->pts -= start_time_tb;
      pkt->dts -= start_time_tb;
    }
    ASSERT(pkt->stream_index == stream_index);
    pkt->stream_index = out_stream->index;
    av_packet_rescale_ts(pkt, in_stream->time_base, out_stream->time_base);
    ASSERT(av_interleaved_write_frame(ofmt_ctx_, pkt) == 0);
  }
  ASSERT(av_interleaved_write_frame(ofmt_ctx_, nullptr) == 0);

  // write trailer
  av_write_trailer(ofmt_ctx_);

  // return vector
  return output_.output_;
}

//
// main
//

int main(int argc, const char** argv) {
  // parse arguments
  utils::Cli cli{argc, argv};
  auto in_file = cli.argument<std::string>("--in");
  auto out_file = cli.argument<std::string>("--out");
  auto start_time = cli.argument<double>("--start-time").value_or(-1);
  auto end_time = cli.argument<double>("--end-time").value_or(-1);
  if (!in_file || !out_file) {
    std::cout << cli.help() << std::endl;
    return 1;
  }

  // read data
  auto in_data = utils::readFile(in_file.value());

  // process
  auto output = convert(in_data, "opus", start_time, end_time);

  // write data
  utils::writeFile(out_file.value(), output);
}
