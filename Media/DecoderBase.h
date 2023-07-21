#pragma once

#include <string>

#include "Utils/Utils.h"

#define TYPE_STR av_get_media_type_string(mMediaType)
namespace ted {
class DecoderBase {
public:
  explicit DecoderBase(std::string mediaFile, AVMediaType type);

  virtual ~DecoderBase();

  virtual int init();

  virtual int seek(int64_t timestampUs);

  virtual int getNextFrame(std::shared_ptr<AVFrame> &frame) = 0;

protected:
  int decodeLoopOnce();

  std::string mPath;
  AVMediaType mMediaType = AVMEDIA_TYPE_UNKNOWN;

  AVFormatContext *mFormatContext = nullptr;
  AVCodecContext *mCodecContext = nullptr;
  int mStreamIndex = -1;

  AVFrame *mFrame = nullptr;
  AVPacket *mPacket = nullptr;
};
}
