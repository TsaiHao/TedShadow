#include "AudioDecoder.h"
#include "DecoderBase.h"
#include "Utils/Utils.h"

#include <cassert>
#include <utility>

using ted::AudioDecoder;
using ted::AudioParam;

AudioDecoder::AudioDecoder(std::string mediaFile)
    : DecoderBase(std::move(mediaFile), AVMEDIA_TYPE_AUDIO) {
  logger.info("Audio decoder created {}", mPath);
}

AudioDecoder::~AudioDecoder() {
  logger.info("Audio decoder destroyed {}", mPath);
}

int AudioDecoder::init() {
  int ret = DecoderBase::init();

  if (ret == 0) {
    logger.info("Audio decoder is ready");
  }

  return 0;
}

int AudioDecoder::seek(int64_t timestampUs) {
  return DecoderBase::seek(timestampUs);
}

int AudioDecoder::getNextFrame(std::shared_ptr<AVFrame>& frame) {
  int ret = decodeLoopOnce();

  if (ret == 0 && mFrame->linesize[0] > 0) {
    auto *interleaved = interleaveSamples(mFrame);
    frame = {interleaved, [](AVFrame *f) {
      av_frame_free(&f);
    }};
    return 0;
  } else {
    return ret;
  }
}

AudioParam AudioDecoder::getAudioParam() const {
  assert(mFormatContext != nullptr && mStreamIndex >= 0);

  AudioParam param;

  param.sampleRate =
      mFormatContext->streams[mStreamIndex]->codecpar->sample_rate;
  param.channels =
      mFormatContext->streams[mStreamIndex]->codecpar->ch_layout.nb_channels;
  switch (mFormatContext->streams[mStreamIndex]->codecpar->format) {
  case AV_SAMPLE_FMT_S16:
    param.sampleFormat = AudioFormat::Int16;
    break;
  case AV_SAMPLE_FMT_S32:
    param.sampleFormat = AudioFormat::Int32;
    break;
  case AV_SAMPLE_FMT_FLT:
    param.sampleFormat = AudioFormat::Float32;
    break;
  case AV_SAMPLE_FMT_FLTP:
    param.sampleFormat = AudioFormat::Float32_Planar;
    break;
  case AV_SAMPLE_FMT_S16P:
    param.sampleFormat = AudioFormat::Int16_Planar;
    break;
  case AV_SAMPLE_FMT_S32P:
    param.sampleFormat = AudioFormat::Int32_Planar;
  default:
    logger.error(
        "Audio decoder failed to get audio param: {}, whose format is {}",
        mPath,
        av_get_sample_fmt_name(
            (AVSampleFormat)mFormatContext->streams[mStreamIndex]
                ->codecpar->format));
    break;
  }

  return param;
}
ted::Time AudioDecoder::convertTime(int64_t timestampUs) const {
  return ted::Time::fromAVTime(timestampUs, mFormatContext->streams[mStreamIndex]->time_base);
}
