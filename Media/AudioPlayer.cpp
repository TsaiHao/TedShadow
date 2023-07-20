#include "AudioPlayer.h"
#include "Utils/Utils.h"

using ted::AudioPlayer;

AudioPlayer::AudioPlayer() { logger.info("create AudioPlayer"); }

AudioPlayer::~AudioPlayer() {
  logger.info("destroy AudioPlayer");
  SDL_CloseAudioDevice(mDeviceID);
}

void AudioPlayer::audioCallback(void *userData, Uint8 *stream, int len) {
  auto player = static_cast<AudioPlayer *>(userData);
  auto &buffer = player->mAudioBuffer;

  std::lock_guard<std::mutex> lock(player->mBufferMutex);

  if (buffer.empty()) {
    logger.info("Audio producer is too slow");
    memset(stream, 0, len);
    return;
  }

  auto size = len / sizeof(float);
  auto readSize = std::min(size, buffer.size());
  memcpy(stream, buffer.data(), readSize * sizeof(float));
  buffer.erase(buffer.begin(), buffer.begin() + readSize);
  if (readSize < size) {
    memset(stream + readSize * sizeof(float), 0,
           (size - readSize) * sizeof(float));
  }
  player->mBufferCond.notify_all();
}

int AudioPlayer::init() {
  if (mStatus != SDL_AUDIO_STOPPED) {
    logger.error("AudioPlayer is not stopped.");
    return -1;
  }

  SDL_AudioSpec want, have;
  SDL_zero(want);
  want.freq = 48000;
  want.format = AUDIO_F32;
  want.channels = 2;
  want.samples = AUDIO_CALLBACK_SIZE;
  want.callback = audioCallback;
  want.userdata = this;

  mDeviceID = SDL_OpenAudioDevice(nullptr, 0, &want, &have,
                                  SDL_AUDIO_ALLOW_FORMAT_CHANGE);
  if (mDeviceID == 0) {
    logger.error("Failed to open audio: {}", SDL_GetError());
    return -1;
  }

  if (have.format != want.format) {
    logger.error("We didn't get Float32 audio format.");
    return -1;
  }

  mSpec = have;
  mStatus = SDL_AUDIO_PAUSED;

  return 0;
}

int AudioPlayer::play() {
  if (mStatus == SDL_AUDIO_PLAYING || mStatus == SDL_AUDIO_STOPPED) {
    logger.error("AudioPlayer is in wrong status {}.", (int)mStatus);
    return -1;
  }

  SDL_PauseAudioDevice(mDeviceID, 0);
  mStatus = SDL_AUDIO_PLAYING;

  return 0;
}

int AudioPlayer::pause() {
  if (mStatus == SDL_AUDIO_PAUSED || mStatus == SDL_AUDIO_STOPPED) {
    logger.error("AudioPlayer is in wrong status {}.", (int)mStatus);
    return -1;
  }

  SDL_PauseAudioDevice(mDeviceID, 1);
  mStatus = SDL_AUDIO_PAUSED;

  return 0;
}

int ted::AudioPlayer::enqueue(const std::vector<float> &data) {
  std::unique_lock lock(mBufferMutex);

  while (mAudioBuffer.size() > MAX_AUDIO_BUFFER_SIZE) {
    mBufferCond.wait(lock);
  }
  mAudioBuffer.insert(mAudioBuffer.end(), data.begin(), data.end());

  return 0;
}
