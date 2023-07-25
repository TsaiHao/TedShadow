#pragma once

#include "DecoderBase.h"

namespace ted {

class SubtitleDecoder: public DecoderBase {
public:
  explicit SubtitleDecoder(std::string mediaFile);

  ~SubtitleDecoder() override;

  int init() override;

  int seek(int64_t timestampUs) override;

  int getNextFrame(std::shared_ptr<AVFrame> &frame) override; 

  int getNextSubtitle(std::shared_ptr<AVSubtitle> &subtitle);
};

}
