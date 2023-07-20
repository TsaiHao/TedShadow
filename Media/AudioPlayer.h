#pragma once

#include <vector>
#include <mutex>
#include <condition_variable>

#include <SDL2/SDL.h>

namespace ted {
class AudioPlayer {
public:
  AudioPlayer();
  ~AudioPlayer();

  int init();

  int play();

  int pause();

  int enqueue(const std::vector<float> &data);

private:
  static constexpr int AUDIO_CALLBACK_SIZE = 4096;
  static constexpr int MAX_AUDIO_BUFFER_SIZE = AUDIO_CALLBACK_SIZE * 10;
  static void audioCallback(void *userData, Uint8 *stream, int len);

  SDL_AudioDeviceID mDeviceID = 0;
  SDL_AudioSpec mSpec {};

  SDL_AudioStatus mStatus = SDL_AUDIO_STOPPED;

  std::mutex mBufferMutex;
  std::condition_variable mBufferCond;
  std::vector<float> mAudioBuffer;
};
}
