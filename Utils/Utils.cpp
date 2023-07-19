#include <iostream>
#include <fstream>

#include "Utils.h"

using ted::Logger;

Logger ted::logger = Logger(std::cout);

Logger::Logger(std::ostream &os) 
  : os(os) {
}

void Logger::sink(ted::LogLevel level, std::string_view message) {
  const char* levelStr = "log";
  switch (level) {
    case LogLevel::Info:
      levelStr = "info";
      break;
    case LogLevel::Error:
      levelStr = "error";
      break;
  }
  os << "ted " << levelStr << ": " << message << "\n";
}