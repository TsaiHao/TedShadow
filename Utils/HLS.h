#pragma once

#include "Utils/Utils.h"

namespace ted {

struct PlayListItem {
  std::string url;
  std::string codec;
  int bandwidth = 0;
  double framerate = 0;
  int width = 0;
  int height = 0;
};

struct PlayListItemAudio {
  std::string url;
  std::string name;
};

class HLSParser {
public:
  explicit HLSParser(std::string url);

  int init();

  [[nodiscard]] std::vector<PlayListItem> getPlayList() const;

  [[nodiscard]] std::vector<PlayListItemAudio> getAudioPlayList() const;

  int downloadPlayListIndex(size_t index, std::string localPath);

  int downloadAudioByName(std::string name, std::string localPath);

private:
  std::string mUrl;
  std::string mPlayList;
  std::vector<PlayListItem> mPlayListItems;
  std::vector<PlayListItemAudio> mAudioPlayListItems;
};

class FFmpegHLSDownloader {
public:
  FFmpegHLSDownloader(std::string url, std::string localPath);

  ~FFmpegHLSDownloader();

  int init();

  int download();

private:
  std::string mUrl;
  std::string mLocalPath;
};

} // namespace ted