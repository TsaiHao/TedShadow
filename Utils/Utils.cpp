#include <iostream>
#include <fstream>

#include "Utils.h"

using ted::Logger;

void Logger::sink(std::string_view message) {
  std::cout << message << std::endl;
}