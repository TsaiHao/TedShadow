#pragma once

#include <sstream>
#include <vector>

#include "Media/AudioDecoder.h"
#include "Media/AudioPlayer.h"
#include "Media/SubtitleDecoder.h"
#include "Utils/Utils.h"

class TedController {
public:
  explicit TedController(std::string url);

  int play();

  int seekByIndex(int64_t index);

private:
  std::string mUrl;
  std::string mMediaFile;

  ted::AudioPlayer mPlayer;
  std::unique_ptr<ted::AudioDecoder> mAudioDecoder;

  std::vector<ted::Subtitle> mSubtitles;
  decltype(mSubtitles)::size_type mSubtitleIndex = 0;
};
