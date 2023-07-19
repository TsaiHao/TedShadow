#pragma once

#include <libavutil/frame.h>
extern "C" {
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include <libavutil/avutil.h>
}

#include <vector>
#include <string>
#include <unordered_map>

#include "Utils/Utils.h"

namespace ted {

class AudioDecoder {
public:
  explicit AudioDecoder(std::string mediaFile);

  ~AudioDecoder();

  int init();

  int seek(int64_t timestampUs);

  std::vector<float> getNextFrame();

  [[nodiscard]] AudioParam getAudioParam() const;

private:
  std::string mPath;

  AVFormatContext* mFormatContext = nullptr;
  AVCodecContext* mCodecContext = nullptr;
  int streamIndex = -1;

  AVFrame *mFrame = nullptr;
  AVPacket *mPacket = nullptr;
};

}
