#include <curl/curl.h>
#include <fstream>
#include <iostream>
#include <stdint.h>

extern "C" {
#include <libavutil/avutil.h>
}
#include "Utils.h"

using ted::Logger;

Logger ted::logger = Logger(std::cout);

Logger::Logger(std::ostream &os) : os(os) {}

void Logger::sink(ted::LogLevel level, std::string_view message) {
  const char *levelStr = "log";
  switch (level) {
  case LogLevel::Info:
    levelStr = "info";
    break;
  case LogLevel::Error:
    levelStr = "error";
    break;
  }
  os << "ted " << levelStr << ": " << message << std::endl;
}

using ted::SimpleDownloader;

SimpleDownloader::SimpleDownloader(std::string url, std::string localPath)
    : mUrl(std::move(url)), mLocalPath(std::move(localPath)) {
  logger.info("SimpleDownloader url: {}", mUrl);
}

SimpleDownloader::~SimpleDownloader() {
  if (mCurl != nullptr) {
    curl_easy_cleanup(mCurl);
  }
}

int SimpleDownloader::setOption(CURLoption curlOption, void *value) {
  if (mCurl == nullptr) {
    logger.error("curl not init {}", mUrl);
    return -1;
  }
  CURLcode res = curl_easy_setopt(mCurl, curlOption, value);
}

int SimpleDownloader::init() {
  if (mCurl != nullptr) {
    logger.error("try to reinit curl {}", mUrl);
  }
  mCurl = curl_easy_init();
  if (mCurl == nullptr) {
    logger.error("curl_easy_init failed {}", mUrl);
    return -1;
  }

  mFile = fopen(mLocalPath.c_str(), "wb");
  if (mFile == nullptr) {
    logger.error("open file failed {}", mLocalPath);
    return -1;
  }

  curl_easy_setopt(mCurl, CURLOPT_URL, mUrl.c_str());
  curl_easy_setopt(mCurl, CURLOPT_WRITEFUNCTION, nullptr);
  curl_easy_setopt(mCurl, CURLOPT_WRITEDATA, mFile);
  curl_easy_setopt(mCurl, CURLOPT_NOPROGRESS, 0L);
  curl_easy_setopt(mCurl, CURLOPT_FOLLOWLOCATION, 1L);
  curl_easy_setopt(mCurl, CURLOPT_FAILONERROR, 1L);
  // curl_easy_setopt(mCurl, CURLOPT_PROXY, "http://127.0.0.1:7890");

  return 0;
}

int SimpleDownloader::download() {
  if (mCurl == nullptr) {
    logger.error("curl not init {}", mUrl);
    return -1;
  }

  CURLcode res = curl_easy_perform(mCurl);
  if (res != CURLE_OK) {
    logger.error("curl_easy_perform failed {}", mUrl);
    return -1;
  }
  fclose(mFile);

  return 0;
}

std::string SimpleDownloader::getLocalPath() const { return mLocalPath; }

std::string ted::getFFmpegErrorStr(int error) {
  std::string ret(100, '0');
  av_strerror(error, ret.data(), ret.size());
  return ret;
}