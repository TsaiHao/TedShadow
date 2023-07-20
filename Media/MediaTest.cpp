#include <catch2/catch_test_macros.hpp>
#include <curl/curl.h>

#include <iostream>
#include <string>
#include <vector>

#include <sys/fcntl.h>
#include <sys/stat.h>
#include <unistd.h>

#include "AudioDecoder.h"
#include "AudioPlayer.h"
#include "Utils/Utils.h"

#include <sstream>

TEST_CASE("logger output", "[logger]") {
  std::stringstream ss;
  ted::Logger logger(ss);
  logger.info("hello world");
  REQUIRE(ss.str() == "ted info: hello world\n");

  ss.str("");
  logger.error("test log {}", "log content");
  REQUIRE(ss.str() == "ted error: test log log content\n");
}

static std::string url =
    "https://download.ted.com/products/168016.mp4?apikey=acme-roadrunner";
static std::string local = "/tmp/test.mp4";

TEST_CASE("test curl downloading ted talks", "[downloader]") {

  ted::SimpleDownloader downloader(url, local);
  downloader.init();
  downloader.download();

  struct stat file_info {};
  stat(local.c_str(), &file_info);
  REQUIRE(file_info.st_size > 0);
}

#define DOWNLOAD_TEST_VIDEO                                                    \
  if (access(local.c_str(), F_OK) != 0) {                                      \
    ted::logger.info("file not exist, downloading");                           \
    ted::SimpleDownloader downloader(url, local);                              \
    REQUIRE(downloader.init() == 0);                                           \
    REQUIRE(downloader.download() == 0);                                       \
  }

TEST_CASE("test audio format retriever", "[audio]") {
  DOWNLOAD_TEST_VIDEO

  ted::AudioDecoder decoder(local);
  REQUIRE(decoder.init() == 0);

  auto audioParam = decoder.getAudioParam();
  REQUIRE(audioParam.sampleRate == 48000);
  REQUIRE(audioParam.channels == 2);
  REQUIRE(audioParam.sampleFormat == ted::AudioFormat::Float32_Planar);
}

TEST_CASE("test audio decoder", "[audio]") {
  DOWNLOAD_TEST_VIDEO

  ted::AudioDecoder decoder(local);
  REQUIRE(decoder.init() == 0);

  auto audioFrame = decoder.getNextFrame();
  REQUIRE(audioFrame.size() > 1);
}

TEST_CASE("test audio player", "[audio]") {
  DOWNLOAD_TEST_VIDEO

  REQUIRE(SDL_Init(SDL_INIT_AUDIO) == 0);

  ted::AudioDecoder decoder(local);
  REQUIRE(decoder.init() == 0);

  auto audioPlayer = ted::AudioPlayer();
  REQUIRE(audioPlayer.init() == 0);

  for (int i = 0; i < 48000 / 1024 * 5; ++i) {
    auto audioFrame = decoder.getNextFrame();
    REQUIRE(audioFrame.size() > 1);

    audioPlayer.enqueue(audioFrame);
    if (i == 8) {
      REQUIRE(audioPlayer.play() == 0);
    }
  }

  REQUIRE(audioPlayer.pause() == 0);
}