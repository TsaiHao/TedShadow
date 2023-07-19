#pragma once

#include <format>
#include <string>

#ifdef __cpp_lib_format
using std::format;
#else
#include <fmt/format.h>
using fmt::format;
#endif

namespace ted {
enum class LogLevel { Info, Error };

class Logger {
public:
  Logger(std::ostream &os);

  template <typename ...Args>
  void info(std::string_view str, Args &&...args) {
    this->log(LogLevel::Info, str, std::forward<Args>(args)...);
  }

  template <typename ...Args>
  void error(std::string_view str, Args &&...args) {
    this->log(LogLevel::Error, str, std::forward<Args>(args)...);
  }

private:
  void sink(LogLevel level, std::string_view message);

  template <typename ...Args>
  void log(LogLevel level, std::string_view str, Args &&...args) {
    sink(level, vformat(str, std::make_format_args(args...)));
  }

  std::ostream &os;
};
extern Logger logger;

enum class AudioFormat { Float32, Int16, Int32 };
struct AudioParam {
  int sampleRate = 0;
  int channels = 0;
  AudioFormat sampleFormat;
};
} // namespace ted
