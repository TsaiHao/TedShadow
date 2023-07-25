#include "Media/AudioDecoder.h"
#include "Media/SubtitleDecoder.h"
#include <iostream>

int main() {
  std::string mediaFile = "/tmp/test.mp4";
  ted::SubtitleDecoder subtitleDecoder(mediaFile);

  int ret;
  ret = subtitleDecoder.init();
  if (ret != 0) {
    return -1;
  }

  std::vector<std::shared_ptr<AVSubtitle>> subtitles;
  ted::logger.info("parsing subtitles");
  while (true) {
    std::shared_ptr<AVSubtitle> subtitle;
    ret = subtitleDecoder.getNextSubtitle(subtitle);
    if (ret != 0) {
      break;
    }
    if (subtitle == nullptr) {
      ted::logger.error("subtitle is null");
      break;
    }
    subtitles.push_back(subtitle);
  }

  ted::logger.info("got {} subtitle frames", subtitles.size());
  subtitles.clear();

  return 0;
}