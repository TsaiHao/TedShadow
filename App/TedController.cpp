#include <fstream>
#include <sys/stat.h>
#include <unistd.h>

#include "TedController.h"
#include "Utils/HLS.h"

using ted::logger;

[[maybe_unused]] static std::vector<ted::Subtitle>
retrieveAllSubtitlesFromAss(const std::string &mediaFile) {
  ted::MediaSubtitleDecoder subtitleDecoder(mediaFile);

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

static const char *CacheDir = "./.cacheMedias";

static inline std::string getCacheFile(const std::string &url) {
  auto file = std::hash<std::string>{}(url);
  std::stringstream ss;
  ss << std::hex;
  ss << CacheDir << "/" << file << ".wav";
  return ss.str();
}

static std::vector<ted::Subtitle> fetchTedTalk(const std::string &url) {
  std::string html;
  ted::SimpleDownloader downloader(url, &html);
  downloader.init();
  downloader.download();

  auto cacheFile = getCacheFile(url);
  if (access(cacheFile.c_str(), F_OK) != 0) {
    auto m3u8 = ted::retrieveM3U8UrlFromTalkHtml(html);
    logger.info("fetching media resources from {}", m3u8);

    ted::HLSParser parser(m3u8);
    parser.init();
    if (parser.getAudioPlayList().empty()) {
      throw std::runtime_error("no audio playlist");
    }
    parser.downloadAudioByName("medium", getCacheFile(url));
  }

  std::vector<ted::Subtitle> subtitles =
      ted::retrieveSubtitlesFromTranscript(html);

  return subtitles;
}

TedController::TedController(std::string url)
    : mUrl(std::move(url)), mMediaFile(getCacheFile(mUrl)) {
  if (access(CacheDir, F_OK) != 0) {
    mkdir(CacheDir, 0777);
  }
  mSubtitles = fetchTedTalk(mUrl);

  mAudioDecoder = std::make_unique<ted::AudioDecoder>(mMediaFile);
  mAudioDecoder->init();
  mPlayer.init(mAudioDecoder->getAudioParam());
  mPlayer.play();
}

int TedController::play() {
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

int TedController::seekByIndex(int64_t index) {
  if (index < 0 || static_cast<size_t>(index) >= mSubtitles.size()) {
    logger.error("invalid index {}", index);
    return -1;
  }

  auto &subtitle = mSubtitles[index];
  logger.info("seeking to subtitle\n {}", subtitle.text);

  mAudioDecoder->seek(subtitle.start.us());

  return 0;
}
