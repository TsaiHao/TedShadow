#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>

#include <memory>
#include <numeric>
#include <string>

#include <sys/stat.h>
#include <unistd.h>

#include "AudioDecoder.h"
#include "AudioPlayer.h"
#include "SubtitleDecoder.h"
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

TEST_CASE("test audio interleaving", "[audio]") {
  auto *frame = av_frame_alloc();
  frame->format = AV_SAMPLE_FMT_FLTP;
  frame->nb_samples = 1024;
  frame->ch_layout.nb_channels = 2;
  frame->linesize[0] = 1024 * 4;
  frame->data[0] = (uint8_t *)av_malloc(frame->linesize[0]);
  frame->data[1] = (uint8_t *)av_malloc(frame->linesize[0]);

  std::iota((float *)frame->data[0], (float *)frame->data[0] + 1024, 0);
  std::iota((float *)frame->data[1], (float *)frame->data[1] + 1024, 1024);

  auto *interleaved = ted::interleaveSamples(frame);

  REQUIRE(interleaved->format == AV_SAMPLE_FMT_FLT);
  REQUIRE(interleaved->nb_samples == 1024);
  REQUIRE(interleaved->ch_layout.nb_channels == 2);
  REQUIRE(interleaved->linesize[0] == 1024 * 4 * 2);
  REQUIRE(interleaved->data[0] != frame->data[0]);
  for (int i = 0; i < 1024; ++i) {
    REQUIRE(((float *)interleaved->data[0])[i * 2] == i);
    REQUIRE(((float *)interleaved->data[0])[i * 2 + 1] == i + 1024);
  }

  av_frame_free(&frame);
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

  std::shared_ptr<AVFrame> audioFrame;
  int ret = decoder.getNextFrame(audioFrame);

  REQUIRE(ret == 0);
  REQUIRE(audioFrame != nullptr);

  std::shared_ptr<AVFrame> audioFrame2;
  ret = decoder.seek(0);
  REQUIRE(ret == 0);

  ret = decoder.getNextFrame(audioFrame2);
  REQUIRE(ret == 0);
  REQUIRE(audioFrame2->pts == audioFrame->pts);

  auto *data = (float *)audioFrame->data[0];
  auto *data2 = (float *)audioFrame2->data[0];
  for (int i = 0; i < 1024; ++i) {
    REQUIRE_THAT(data[i], Catch::Matchers::WithinAbs(0.0f, 0.0001f));
  }
}

TEST_CASE("test audio player", "[audio]") {
  DOWNLOAD_TEST_VIDEO

  REQUIRE(SDL_Init(SDL_INIT_AUDIO) == 0);

  ted::AudioDecoder decoder(local);
  REQUIRE(decoder.init() == 0);

  auto audioPlayer = ted::AudioPlayer();
  REQUIRE(audioPlayer.init() == 0);

  ted::logger.info("start playing 20 seconds sound");

  for (int i = 0; i < 48000 / 1024 * 20; ++i) {
    std::shared_ptr<AVFrame> audioFrame;
    int ret = decoder.getNextFrame(audioFrame);

    REQUIRE(ret == 0);
    REQUIRE(audioFrame != nullptr);

    audioPlayer.enqueue(audioFrame);
    if (i == 8) {
      REQUIRE(audioPlayer.play() == 0);
    }
  }

  REQUIRE(audioPlayer.pause() == 0);
}

TEST_CASE("test subtitle", "[subtitle]") {
  DOWNLOAD_TEST_VIDEO

  ted::SubtitleDecoder decoder(local);
  REQUIRE(decoder.init() == 0);

  ted::Subtitle subtitle;
  int ret = decoder.getNextSubtitle(subtitle);

  REQUIRE(ret == 0);
  REQUIRE(!subtitle.text.empty());
  REQUIRE(subtitle.start != subtitle.end);
}

TEST_CASE("test ted fetch", "[downloader]") {
  std::string buffer;
  auto downloader = ted::SimpleDownloader(
      "https://www.ted.com/talks/vinu_daniel_how_today_s_scraps_will_be_tomorrow_s_sustainable_buildings",
      "./test.html");
  REQUIRE(downloader.init() == 0);
  REQUIRE(downloader.download() == 0);
}