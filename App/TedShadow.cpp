#include "TedController.h"

using ted::logger;

int main() {
  std::string mediaFile = "https://www.ted.com/talks/"
                          "francis_de_los_reyes_how_the_water_you_flush_becomes_the_water_you_drink";

  TedController controller(mediaFile);
  for (int i = 0; i < 10; ++i) {
    controller.play();
  }

  return 0;
}