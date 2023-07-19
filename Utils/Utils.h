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
  void sink(std::string_view message);

  template <typename ...Args>
  void _log(LogLevel level, std::string_view str, Args &&...args) {
    sink(format(str, std::forward<Args>(args)...));
  }

  template <typename ...Args>
  void info(std::string_view str, Args &&...args) {
    _log(LogLevel::Info, str, std::forward<Args>(args)...);
  }

  template <typename ...Args>
  void error(std::string_view format, Args &&...args) {
    _log(LogLevel::Error, format, std::forward<Args>(args)...);
  }
};
extern Logger logger;

enum class AudioFormat { Float32, Int16, Int32 };
struct AudioParam {
  int sampleRate = 0;
  int channels = 0;
  AudioFormat sampleFormat;
};
} // namespace ted
