#pragma once

#include "DecoderBase.h"

namespace ted {

struct Subtitle {
  std::string text;
  Time start = Time(0);
  Time end = Time(0);

  Subtitle& merge(const Subtitle& next);

  [[nodiscard]] std::string toString() const;
  static Subtitle fromString(const std::string& str);
};
bool operator==(const Subtitle& lhs, const Subtitle& rhs);

class SubtitleDecoder : public DecoderBase {
public:
  explicit SubtitleDecoder(std::string mediaFile);

  ~SubtitleDecoder() override;

  int init() override;

  int seek(int64_t timestampUs) override;

  int getNextFrame(std::shared_ptr<AVFrame> &frame) override; 

  int getNextSubtitle(Subtitle &subtitle);
};

std::vector<Subtitle> retrieveSubtitlesFromTranscript(const std::string& html);

std::vector<Subtitle> mergeSubtitles(const std::vector<Subtitle> &subtitles);
}
