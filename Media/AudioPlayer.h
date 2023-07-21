#pragma once

#include <vector>
#include <deque>
#include <mutex>
#include <condition_variable>

#include <SDL.h>

#include "Utils/Utils.h"

namespace ted {
class AudioPlayer {
public:
  AudioPlayer();
  ~AudioPlayer();

  int init();

  int play();

  int pause();

  int enqueue(const std::shared_ptr<AVFrame> &frame);

private:
  struct AudioCache {
    std::shared_ptr<AVFrame> frame = nullptr;
    int progress = 0;
  };

  static constexpr int AUDIO_CALLBACK_SIZE = 4096;
  static constexpr int MAX_AUDIO_BUFFER_SIZE = 10;
  static void audioCallback(void *userData, Uint8 *stream, int len);

  SDL_AudioDeviceID mDeviceID = 0;
  SDL_AudioSpec mSpec {};

  SDL_AudioStatus mStatus = SDL_AUDIO_STOPPED;

  std::mutex mBufferMutex;
  std::condition_variable mBufferCond;
  std::deque<AudioCache> mAudioBuffer;
};
}
