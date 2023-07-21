#pragma once

#include <format>
#include <string>

#include <curl/curl.h>

#ifdef __cpp_lib_format
using std::format;
using std::make_format_args;
#else
#include <fmt/format.h>
using fmt::format;
using fmt::make_format_args;
#endif

extern "C" {
#include <libavutil/avutil.h>
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/frame.h>
#include <libswresample/swresample.h>
}

namespace ted {
enum class LogLevel { Info, Error };

class Logger {
public:
  explicit Logger(std::ostream &os);

  template <typename... Args> void info(std::string_view str, Args &&...args) {
    this->log(LogLevel::Info, str, std::forward<Args>(args)...);
  }

  template <typename... Args> void error(std::string_view str, Args &&...args) {
    this->log(LogLevel::Error, str, std::forward<Args>(args)...);
  }

private:
  void sink(LogLevel level, std::string_view message);

  template <typename... Args>
  void log(LogLevel level, std::string_view str, Args &&...args) {
    sink(level, vformat(str, make_format_args(args...)));
  }

  std::ostream &os;
};
extern Logger logger;

enum class AudioFormat {
  None,
  Float32,
  Int16,
  Int32,
  Float32_Planar,
  Int16_Planar,
  Int32_Planar
};
struct AudioParam {
  int sampleRate = 0;
  int channels = 0;
  AudioFormat sampleFormat = AudioFormat::Float32;
};

AVFrame *interleaveSamples(AVFrame *frame);

class SimpleDownloader {
public:
  SimpleDownloader(std::string url, std::string localPath);

  ~SimpleDownloader();

  int setOption(CURLoption curlOption, void *value);

  int init();

  int download();

  [[nodiscard]] std::string getLocalPath() const;

private:
  std::string mUrl;
  std::string mLocalPath;

  CURL *mCurl = nullptr;
  FILE *mFile = nullptr;
};

std::string getFFmpegErrorStr(int error);

} // namespace ted
