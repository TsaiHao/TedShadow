#include "SubtitleDecoder.h"
#include "Utils/Utils.h"

using ted::Subtitle;

int Subtitle::merge(const Subtitle &next) {
  if (next.start.us() > end.us()) {
    return -1;
  }

  if (next.start.us() < end.us()) {
    text += "\n";
  }

  text += next.text;
  end = next.end;

  return 0;
}

using ted::SubtitleDecoder;

SubtitleDecoder::SubtitleDecoder(std::string mediaFile)
    : DecoderBase(std::move(mediaFile), AVMEDIA_TYPE_SUBTITLE) {}

SubtitleDecoder::~SubtitleDecoder() = default;

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

static std::string parseAssDialogue(const char *ass) {
  std::string res;
  int sepPos = 0;
  int nComma = 0;
  int len = strlen(ass);

  for (int i = 0; i < len; ++i) {
    if (ass[i] == ',') {
      ++nComma;
      if (nComma == 8) {
        sepPos = i;
        break;
      }
    }
  }

  if (nComma == 8) {
    res = std::string(&ass[sepPos + 1], len - sepPos - 1);
  }

  return res;
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

    ret = avcodec_decode_subtitle2(mCodecContext, &avSub, &gotSub, mPacket);
    if (ret < 0) {
      logger.error("Subtitle decode failed: {}", getFFmpegErrorStr(ret));
      return ret;
    }
    if (gotSub == 0) {
      continue;
    }

    pts = mPacket->pts;
    duration = mPacket->duration;
  } while (gotSub == 0);

  if (avSub.num_rects != 1) {
    logger.error("Subtitle decode failed: invalid subtitle rects {}",
                 avSub.num_rects);
    return -1;
  }

  subtitle.text = parseAssDialogue(avSub.rects[0]->ass);
  subtitle.start = Time::fromUs(pts);
  subtitle.end = Time::fromUs(pts + duration);

  avsubtitle_free(&avSub);

  return 0;
}