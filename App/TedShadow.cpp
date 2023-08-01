#include "Media/AudioDecoder.h"
#include "Media/AudioPlayer.h"
#include "Media/SubtitleDecoder.h"

#include <unistd.h>
#include <sys/stat.h>
#include <iostream>
#include <fstream>
#include <sstream>

using ted::logger;

static std::vector<ted::Subtitle>
retrieveAllSubtitles(const std::string &mediaFile) {
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

static const char* CacheDir = "./.cacheMedias";

static inline std::string getCacheFile(const std::string& url) {
  auto file = std::hash<std::string>{}(url);
  std::stringstream ss;
  ss << std::hex;
  ss << CacheDir << "/" << file;
  return ss.str();
}

class TedController {
public:
  explicit TedController(std::string url) {
    auto getIter = url.find_last_of('?');
    if (getIter != std::string::npos) {
      //url = url.substr(0, getIter);
    }
    mUrl = std::move(url);
    mMediaFile = getCacheFile(mUrl);

    if (access(CacheDir, F_OK) != 0) {
      mkdir(CacheDir, 0777);
    }
    if (!std::ifstream(mMediaFile).good()) {
      ted::SimpleDownloader downloader(mUrl, mMediaFile);
      downloader.init();
      downloader.download();
    }
    mAudioDecoder = std::make_unique<ted::AudioDecoder>(mMediaFile);
    mAudioDecoder->init();
    mPlayer.init();
    mPlayer.play();

    mSubtitles = retrieveAllSubtitles(mMediaFile);
    assert(!mSubtitles.empty());
  }

  /**
   * Plays the next subtitle in the sequence.
   *
   * If there are no more subtitles available, an error message is logged and -1
   * is returned. Otherwise, the subtitle is logged, and if the time difference
   * between the subtitle's start time and the current audio decoder time is
   * greater than 1024/48000, a seek operation is performed. The audio frames
   * are obtained from the audio decoder and enqueued to the player until the
   * end of the current subtitle is reached or there are no more frames
   * available.
   *
   * @return 0 if the play operation is successful, -1 otherwise.
   */
  int play() {
    if (mSubtitleIndex >= mSubtitles.size()) {
      logger.error("no more subtitles");
      return -1;
    }

    auto &subtitle = mSubtitles[mSubtitleIndex];
    logger.info("start playing\n {}", subtitle.text);

    int64_t timeDiff =
        std::abs(subtitle.start.us() - mAudioDecoder->getCurrentTime().us());
    if (timeDiff > 1024 / 48000) {
      logger.info("time diff {} is too large, seeking", timeDiff);
      mAudioDecoder->seek(subtitle.start.us());
    }

    while (true) {
      std::shared_ptr<AVFrame> frame;
      int ret = mAudioDecoder->getNextFrame(frame);
      if (ret != 0) {
        logger.error("failed to get next frame");
        return -1;
      }

      if (frame == nullptr) {
        logger.info("no more frames");
        break;
      }

      mPlayer.enqueue(frame);
      if (mAudioDecoder->getCurrentTime() >= subtitle.end) {
        break;
      }
    }

    ++mSubtitleIndex;

    return 0;
  }

  int seekByIndex(int64_t index) {
    if (index < 0 || static_cast<size_t>(index) >= mSubtitles.size()) {
      logger.error("invalid index {}", index);
      return -1;
    }

    auto &subtitle = mSubtitles[index];
    logger.info("seeking to subtitle\n {}", subtitle.text);

    mAudioDecoder->seek(subtitle.start.us());

    return 0;
  }

private:
  std::string mUrl;
  std::string mMediaFile;

  ted::AudioPlayer mPlayer;
  std::unique_ptr<ted::AudioDecoder> mAudioDecoder;

  std::vector<ted::Subtitle> mSubtitles;
  decltype(mSubtitles)::size_type mSubtitleIndex = 0;
};

int main() {
  std::string mediaFile = "https://download.ted.com/products/168780.mp4?apikey=acme-roadrunner";

  TedController controller(mediaFile);
  for (int i = 0; i < 10; ++i) {
    controller.play();
  }

  return 0;
}