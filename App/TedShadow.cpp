#include "TedController.h"

using ted::logger;

int main() {
  std::string url = "https://www.ted.com/talks/"
                          "francis_de_los_reyes_how_the_water_you_flush_becomes_the_water_you_drink";

  TedController::GlobalInit();

  auto controller = TedController(url);
  controller.run();

  return 0;
}