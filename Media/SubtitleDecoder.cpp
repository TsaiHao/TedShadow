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

int SubtitleDecoder::getNextSubtitle(Subtitle &subtitle) {
  int ret;
  int gotSub = 1;
  int64_t pts = 0, duration = 0;

  AVSubtitle avSub;
  do {
    do {
      ret = av_read_frame(mFormatContext, mPacket);
      if (ret < 0) {
        logger.error("Subtitle read frame failed: {}", getFFmpegErrorStr(ret));
        return ret;
      }
      if (mPacket->stream_index == mStreamIndex) {
        break;
      }
    } while (true);

    ret = avcodec_decode_subtitle2(mCodecContext, &avSub, &gotSub,
                                   mPacket);
    if (ret < 0) {
      logger.error("Subtitle decode failed: {}", getFFmpegErrorStr(ret));
      return ret;
    }
    if (gotSub == 0) {
      logger.error("Subtitle decode failed: no subtitle in pts {}",
                   mPacket->pts);
    }

    pts = mPacket->pts;
    duration = mPacket->duration;
  } while (gotSub == 0);

  if (avSub.num_rects != 1) {
    logger.error("Subtitle decode failed: invalid subtitle rects {}",
                 avSub.num_rects);
    return -1;
  }

  subtitle.text = std::string(avSub.rects[0]->ass);
  subtitle.start = Time::fromUs(pts);
  subtitle.end = Time::fromUs(pts + duration);

  return 0;
}