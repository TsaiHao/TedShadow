#include <cassert>
#include <curl/curl.h>
#include <fstream>
#include <iostream>
#include <vector>

#include "Utils.h"

using ted::Logger;

namespace ted {
bool operator==(const ted::Time &lhs, const ted::Time &rhs) {
  return lhs.num * rhs.den == rhs.num * lhs.den;
}

bool operator!=(const ted::Time &lhs, const ted::Time &rhs) {
  return !(lhs == rhs);
}
} // namespace ted

// todo: buggy code
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

AVFrame *ted::interleaveSamples(AVFrame *srcFrame) {
  assert(srcFrame->ch_layout.nb_channels == 2);
  int nSample = srcFrame->nb_samples;
  int nChannel = srcFrame->ch_layout.nb_channels;

  AVFrame *dstFrame = av_frame_alloc();
  av_frame_copy_props(dstFrame, srcFrame);
  dstFrame->nb_samples = nSample;
  dstFrame->ch_layout = srcFrame->ch_layout;

  dstFrame->format = AV_SAMPLE_FMT_FLT;
  dstFrame->data[0] = (uint8_t *)av_malloc(nSample * nChannel * sizeof(float));
  dstFrame->linesize[0] = nSample * nChannel * sizeof(float);

  if (srcFrame->format == AV_SAMPLE_FMT_FLT) {
    // already interleaved
    memcpy(dstFrame->data[0], srcFrame->data[0],
           nSample * nChannel * sizeof(float));
    return dstFrame;
  } else if (srcFrame->format == AV_SAMPLE_FMT_FLTP) {
    // interleave data to dst
    for (int i = 0; i < nSample; i++) {
      for (int j = 0; j < nChannel; j++) {
        ((float *)dstFrame->data[0])[i * nChannel + j] =
            ((float *)srcFrame->data[j])[i];
      }
    }
  } else {
    assert(false && "unsupported format");
  }

  return dstFrame;
}

using ted::SimpleDownloader;

SimpleDownloader::SimpleDownloader(std::string url, std::string localPath)
    : mUrl(std::move(url)), mLocalPath(std::move(localPath)) {
  logger.info("SimpleDownloader url: {}", mUrl);
}

SimpleDownloader::SimpleDownloader(std::string url, std::string *buffer)
    : mUrl(std::move(url)), mBuffer(buffer) {}

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
  if (res != CURLE_OK) {
    logger.error("curl_easy_setopt failed {}", mUrl);
    return -1;
  }
  return 0;
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

  curl_easy_setopt(mCurl, CURLOPT_URL, mUrl.c_str());
  if (mBuffer == nullptr) {
    mFile = fopen(mLocalPath.c_str(), "wb");
    if (mFile == nullptr) {
      logger.error("open file failed {}", mLocalPath);
      return -1;
    }
    curl_easy_setopt(mCurl, CURLOPT_WRITEFUNCTION, nullptr);
    curl_easy_setopt(mCurl, CURLOPT_WRITEDATA, mFile);
  } else {
    curl_easy_setopt(mCurl, CURLOPT_WRITEFUNCTION, WriteBufferCallback);
    curl_easy_setopt(mCurl, CURLOPT_WRITEDATA, this);
  }
  curl_easy_setopt(mCurl, CURLOPT_NOPROGRESS, 1L);
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
  if (mFile != nullptr) {
    fclose(mFile);
  }

  return 0;
}

std::string SimpleDownloader::getLocalPath() const { return mLocalPath; }

size_t ted::SimpleDownloader::WriteBufferCallback(void *contents, size_t size,
                                                  size_t nmemb, void *user) {
  size_t realSize = size * nmemb;
  auto *downloader = static_cast<SimpleDownloader *>(user);
  if (downloader->mBuffer == nullptr) {
    logger.error("buffer not init {}", downloader->mUrl);
    return 0;
  }
  downloader->mBuffer->append(static_cast<char *>(contents), realSize);
  return realSize;
}

std::string ted::getFFmpegErrorStr(int error) {
  std::string ret(100, '0');
  av_strerror(error, ret.data(), ret.size());
  return ret;
}
