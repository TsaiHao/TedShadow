#include <sstream>

#include "HLS.h"

using ted::FFmpegHLSDownloader;

FFmpegHLSDownloader::FFmpegHLSDownloader(std::string url, std::string localPath)
    : mUrl(std::move(url)), mLocalPath(std::move(localPath)) {}

FFmpegHLSDownloader::~FFmpegHLSDownloader() = default;

int FFmpegHLSDownloader::init() {
  logger.info("FFmpegHLSDownloader url: {}", mUrl);
  return 0;
}

int FFmpegHLSDownloader::download() {
  logger.info("FFmpegHLSDownloader download: {}", mUrl);
  std::string ffmpeg = "ffmpeg -i " + mUrl + " " + mLocalPath + " -y";

  int ret = system(ffmpeg.c_str());
  if (ret != 0) {
    logger.error("FFmpegHLSDownloader download failed: {}", mUrl);
    return -1;
  }

  return 0;
}

using ted::HLSParser;

HLSParser::HLSParser(std::string url) : mUrl(std::move(url)) {}

static std::string_view removeQuotes(std::string_view str) {
  if (str.starts_with('\"')) {
    str.remove_prefix(1);
  }
  if (str.ends_with('\"')) {
    str.remove_suffix(1);
  }
  return str;
}

int HLSParser::init() {
  SimpleDownloader downloader(mUrl, &mPlayList);

  int ret;
  ret = downloader.init();
  assert(ret == 0);
  ret = downloader.download();
  assert(ret == 0);

  std::string urlBase = mUrl.substr(0, mUrl.find_last_of('/') + 1);
  std::string_view streamTag = "#EXT-X-STREAM-INF";
  std::string_view mediaTag = "#EXT-X-MEDIA";

  std::stringstream ss(mPlayList);
  std::string line;
  while (std::getline(ss, line)) {
    if (line.find(streamTag) != std::string::npos) {
      PlayListItem item;
      std::string url;
      std::getline(ss, url);
      size_t index = streamTag.size();
      do {
        size_t equalIndex = line.find('=', index);
        if (equalIndex == std::string::npos) {
          break;
        }
        size_t commaIndex = line.find(',', equalIndex);
        if (commaIndex == std::string::npos) {
          break;
        }
        std::string key = line.substr(index, equalIndex - index);
        std::string value =
            line.substr(equalIndex + 1, commaIndex - equalIndex - 1);
        if (key == "CODECS") {
          item.codec = value;
        } else if (key == "BANDWIDTH") {
          item.bandwidth = std::stoi(value);
        } else if (key == "FRAME-RATE") {
          item.framerate = std::stod(value);
        } else if (key == "RESOLUTION") {
          size_t xIndex = value.find('x');
          if (xIndex != std::string::npos) {
            item.width = std::stoi(value.substr(0, xIndex));
            item.height = std::stoi(value.substr(xIndex + 1));
          }
        }
        index = commaIndex + 1;
      } while (true);
      item.url = urlBase + url;
      mPlayListItems.push_back(item);
    } else if (line.find(mediaTag) != std::string::npos) {
      PlayListItemAudio item;
      size_t index = mediaTag.size();
      do {
        size_t equalIndex = line.find('=', index);
        if (equalIndex == std::string::npos) {
          break;
        }
        size_t commaIndex = line.find(',', equalIndex);
        if (commaIndex == std::string::npos) {
          break;
        }
        std::string key = line.substr(index, equalIndex - index);
        std::string value =
            line.substr(equalIndex + 1, commaIndex - equalIndex - 2);
        if (key == "TYPE") {
          auto type = trim(value);
          if (type != "AUDIO") {
            break;
          }
        }
        if (key == "URI") {
          item.url = urlBase + std::string(removeQuotes(trim(value)));
        } else if (key == "NAME") {
          item.name = std::string(removeQuotes(trim(value)));
        }
        index = commaIndex + 1;
      } while (true);
      if (item.url.empty()) {
        continue;
      }
      mAudioPlayListItems.push_back(item);
    }
  }

  logger.info("got {} streams from {}", mPlayListItems.size(), mUrl);

  return 0;
}

std::vector<ted::PlayListItem> HLSParser::getPlayList() const {
  return mPlayListItems;
}

std::vector<ted::PlayListItemAudio> HLSParser::getAudioPlayList() const {
  return mAudioPlayListItems;
}

int HLSParser::downloadPlayListIndex(size_t index, std::string localPath) {
  auto &item = mPlayListItems[index];
  FFmpegHLSDownloader downloader(item.url, localPath);

  int ret = downloader.init();
  assert(ret == 0);
  ret = downloader.download();
  assert(ret == 0);
  logger.info("download {} to {}", item.url, localPath);

  return 0;
}

int HLSParser::downloadAudioByName(std::string name, std::string localPath) {
  auto iter = std::find_if(
      mAudioPlayListItems.begin(), mAudioPlayListItems.end(),
      [&name](const PlayListItemAudio &item) { return item.name == name; });
  if (iter == mAudioPlayListItems.end()) {
    throw std::runtime_error("no such audio name");
  }

  FFmpegHLSDownloader downloader(iter->url, localPath);
  int ret = downloader.init();
  assert(ret == 0);
  ret = downloader.download();
  assert(ret == 0);
  logger.info("download {} to {}", iter->url, localPath);

  return 0;
}
