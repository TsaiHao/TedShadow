#pragma once

#include <format>
#include <string>

#ifdef __cpp_lib_format
using std::format;
#else
#include <fmt>
using fmt::format;
#endif

namespace ted {
enum class LogLevel { Info, Error };

class Logger {
public:
  void sink(std::string_view message);

  template <typename ...Args>
  void log(LogLevel level, std::string_view format, Args &&...args) {
    sink(format(format, std::forward<Args>(args)...));
  }

  template <typename ...Args>
  void info(std::string_view format, Args &&...args) {
    log(LogLevel::Info, format, std::forward<Args>(args)...);
  }

  template <typename ...Args>
  void error(std::string_view format, Args &&...args) {
    log(LogLevel::Error, format, std::forward<Args>(args)...);
  }
};
} // namespace ted