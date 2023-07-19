#include <iostream>
#include <fstream>

#include "Utils.h"

using ted::Logger;

Logger ted::logger = Logger();

void Logger::sink(std::string_view message) {
  std::cout << "ted log: " << message << std::endl;
}