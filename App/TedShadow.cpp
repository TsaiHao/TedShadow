#include "Media/AudioDecoder.h"
#include "Media/AudioPlayer.h"
#include "Media/SubtitleDecoder.h"

#include "unistd.h"
#include <iostream>

using ted::logger;

std::vector<ted::Subtitle> retrieveAllSubtitles(const std::string &mediaFile) {
  ted::SubtitleDecoder subtitleDecoder(mediaFile);

  int ret;
  ret = subtitleDecoder.init();
  if (ret != 0) {
    return {};
  }

  std::vector<ted::Subtitle> subtitles;
  ted::logger.info("parsing subtitles");
  while (true) {
    ted::Subtitle subtitle;
    ret = subtitleDecoder.getNextSubtitle(subtitle);
    if (ret != 0) {
      break;
    }
    subtitles.push_back(subtitle);
  }

  ted::logger.info("got {} subtitle frames", subtitles.size());

  return subtitles;
}

int main() {
  int ret;
  std::string mediaFile = "/tmp/test.mp4";
  if (access(mediaFile.c_str(), F_OK) != 0) {
    ted::logger.error("file {} not exist", mediaFile);
    return -1;
  }

  std::vector<ted::Subtitle> subtitles = retrieveAllSubtitles(mediaFile);
  if (subtitles.empty()) {
    ted::logger.error("no subtitles found");
    return -1;
  }

  ted::AudioPlayer player;
  ret = player.init();
  if (ret != 0) {
    ted::logger.error("failed to init audio player");
  }

  ted::AudioDecoder audioDecoder(mediaFile);
  ret = audioDecoder.init();
  if (ret != 0) {
    ted::logger.error("failed to init audio decoder");
  }

  std::shared_ptr<AVFrame> frame;
  auto subIter = subtitles.begin();
  ted::logger.info("start playing\n {}", subIter->text);
  char directive;

  player.play();

  while (true) {
    ret = audioDecoder.getNextFrame(frame);
    if (ret != 0) {
      break;
    }

    if (subIter == subtitles.end()) {
      ted::logger.info("no more subtitles");
      break;
    }
    if (subIter->end < audioDecoder.convertTime(frame->pts)) {
      ted::logger.info("enter a key");
      std::cin >> directive;

      if (directive == 'n') {
        subIter++;
        if (subIter != subtitles.end()) {
          ted::logger.info("subtitle: {}", subIter->text);
        }
      } else if (directive == 'q') {
        break;
      } else {
        ted::logger.info("unknown directive: {}", directive);
        break;
      }
    }

    player.enqueue(frame);
  }

  ted::logger.info("exiting");

  return 0;
}