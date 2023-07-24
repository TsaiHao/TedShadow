#include "SubtitleDecoder.h"
#include "Utils/Utils.h"

using ted::SubtitleDecoder;

SubtitleDecoder::SubtitleDecoder(std::string mediaFile)
    : DecoderBase(std::move(mediaFile), AVMEDIA_TYPE_SUBTITLE) {}

SubtitleDecoder::~SubtitleDecoder() {}

int SubtitleDecoder::init() {
  int ret = DecoderBase::init();
  if (ret < 0) {
    logger.error("Subtitle init failed, please verify your ffmpeg supporting "
                 "mov_text decoder");
    return ret;
  }

  return 0;
}

int SubtitleDecoder::seek(int64_t timestampUs) {
  return DecoderBase::seek(timestampUs);
}

int SubtitleDecoder::getNextFrame(std::shared_ptr<AVFrame> &frame) {
  int ret = decodeLoopOnce();
  if (ret != 0) {
    return ret;
  }

  frame = nullptr;
  return 0;
}

AVSubtitle SubtitleDecoder::getNextSubtitle() {
  AVSubtitle subtitle;
  int ret;
  int gotSub = 1;

  do {
    do {
      ret = av_read_frame(mFormatContext, mPacket);
      if (ret < 0) {
        logger.error("Subtitle read frame failed: {}", getFFmpegErrorStr(ret));
        return subtitle;
      }
      if (mPacket->stream_index == mStreamIndex) {
        break;
      }
    } while (true);

    ret = avcodec_decode_subtitle2(mCodecContext, &subtitle, &gotSub, mPacket);
    if (ret < 0) {
      logger.error("Subtitle decode failed: {}", getFFmpegErrorStr(ret));
    }
    if (gotSub == 0) {
      logger.error("Subtitle decode failed: no subtitle in pts {}",
                   mPacket->pts);
    }
  } while (gotSub == 0);

  return subtitle;
}