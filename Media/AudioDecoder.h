#pragma once

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

  std::shared_ptr<AVFrame> getNextFrame();

  [[nodiscard]] AudioParam getAudioParam() const;

private:
  std::string mPath;

  AVFormatContext* mFormatContext = nullptr;
  AVCodecContext* mCodecContext = nullptr;
  int mStreamIndex = -1;

  AVFrame *mFrame = nullptr;
  AVPacket *mPacket = nullptr;
};

}
