#pragma once

#include "DecoderBase.h"

namespace ted {

struct Subtitle {
  std::string text;
  Time start = Time(0);
  Time end = Time(0);

  int merge(const Subtitle& next);
};

class SubtitleDecoder: public DecoderBase {
public:
  explicit SubtitleDecoder(std::string mediaFile);

  ~SubtitleDecoder() override;

  int init() override;

  int seek(int64_t timestampUs) override;

  int getNextFrame(std::shared_ptr<AVFrame> &frame) override; 

  int getNextSubtitle(Subtitle &subtitle);
};

}
