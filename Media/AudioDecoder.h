#pragma once

#include <vector>
#include <string>
#include <unordered_map>

#include "Utils/Utils.h"
#include "DecoderBase.h"

namespace ted {

class AudioDecoder: public DecoderBase {
public:
  explicit AudioDecoder(std::string mediaFile);

  ~AudioDecoder() override;

  int init() override;

  int seek(int64_t timestampUs) override;

  int getNextFrame(std::shared_ptr<AVFrame>& frame) override;

  [[nodiscard]] AudioParam getAudioParam() const;
};

}
