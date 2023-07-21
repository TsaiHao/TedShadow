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

  int needSample = len / sizeof(float) / player->mSpec.channels;
  size_t dstOffset = 0;

  std::unique_lock lock(player->mBufferMutex);
  while (needSample > 0 && !buffer.empty()) {
    auto &cache = buffer.front();
    auto frame = cache.frame;
    assert(frame->format == AV_SAMPLE_FMT_FLT &&
           frame->ch_layout.nb_channels == 2);
    auto &progress = cache.progress;
    size_t srcOffset = progress * sizeof(float) * player->mSpec.channels;

    size_t copySize = std::min(
        (frame->nb_samples - progress) * sizeof(float) * player->mSpec.channels,
        needSample * sizeof(float) * player->mSpec.channels);

    memcpy(stream + dstOffset, frame->data[0] + srcOffset, copySize);
    dstOffset += copySize;
    needSample -= copySize / sizeof(float) / player->mSpec.channels;
    progress += copySize / sizeof(float) / player->mSpec.channels;

    if (progress == frame->nb_samples) {
      buffer.pop_front();
      player->mBufferCond.notify_one();
    }
  }

  lock.unlock();
  if (needSample > 0) {
    memset(stream + dstOffset, 0,
           needSample * sizeof(float) * player->mSpec.channels);
  }
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

int ted::AudioPlayer::enqueue(const std::shared_ptr<AVFrame> &frame) {
  std::unique_lock lock(mBufferMutex);

  while (mAudioBuffer.size() > MAX_AUDIO_BUFFER_SIZE) {
    mBufferCond.wait(lock);
  }

  AudioCache cache{.frame = frame, .progress = 0};
  mAudioBuffer.push_back(cache);

  return 0;
}
