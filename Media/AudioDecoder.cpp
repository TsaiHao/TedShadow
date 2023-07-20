#include "AudioDecoder.h"
#include "Utils/Utils.h"
#include <libavcodec/avcodec.h>
#include <libavcodec/packet.h>
#include <libavformat/avformat.h>
#include <libavutil/frame.h>

using ted::AudioDecoder;
using ted::AudioParam;

static void interleaveSamples(std::vector<float> &dst,
                                                 AVFrame *frame) {
  assert(frame->ch_layout.nb_channels == 2);
  if (frame->format == AV_SAMPLE_FMT_FLT) {
    dst.resize(frame->nb_samples * frame->ch_layout.nb_channels);
    float *dstPtr = dst.data();
    auto *srcPtr = (float *)frame->data[0];
    memcpy(dstPtr, srcPtr, dst.size() * sizeof(float));
  } else if (frame->format == AV_SAMPLE_FMT_FLTP) {
    // do interleave to dst
    dst.resize(frame->nb_samples * frame->ch_layout.nb_channels);
    float *dstPtr = dst.data();
    auto *srcPtr = (float *)frame->data[0];
    for (int i = 0; i < frame->nb_samples; ++i) {
      for (int j = 0; j < frame->ch_layout.nb_channels; ++j) {
        dstPtr[i * frame->ch_layout.nb_channels + j] =
            srcPtr[j * frame->nb_samples + i];
      }
    }
  }
}

AudioDecoder::AudioDecoder(std::string mediaFile)
    : mPath(std::move(mediaFile)) {
  logger.info("Audio decoder created {}", mPath);
}

AudioDecoder::~AudioDecoder() {
  if (mCodecContext != nullptr) {
    avcodec_free_context(&mCodecContext);
  }
  if (mFormatContext != nullptr) {
    avformat_close_input(&mFormatContext);
  }
  logger.info("Audio decoder destroyed {}", mPath);
}

int AudioDecoder::init() {
  int ret;
  ret = avformat_open_input(&mFormatContext, mPath.c_str(), nullptr, nullptr);
  if (ret < 0) {
    logger.error("Audio decoder failed to open input file: {}", mPath);
    return -1;
  }

  ret = avformat_find_stream_info(mFormatContext, nullptr);
  if (ret < 0) {
    logger.error("Audio decoder failed to find stream info: {}", mPath);
    return -1;
  }

  int audioStreamIndex = av_find_best_stream(mFormatContext, AVMEDIA_TYPE_AUDIO,
                                             -1, -1, nullptr, 0);
  if (audioStreamIndex < 0) {
    logger.error("Audio decoder failed to find audio stream: {}", mPath);
    return -1;
  }
  mStreamIndex = audioStreamIndex;

  for (int i = 0; i < mFormatContext->nb_streams; ++i) {
    if (i == audioStreamIndex) {
      continue;
    }
    mFormatContext->streams[i]->discard = AVDISCARD_ALL;
  }

  AVStream *audioStream = mFormatContext->streams[audioStreamIndex];
  AVCodec const *audioCodec =
      avcodec_find_decoder(audioStream->codecpar->codec_id);
  if (audioCodec == nullptr) {
    logger.error("Audio decoder failed to find audio decoder: {}", mPath);
    return -1;
  }

  mCodecContext = avcodec_alloc_context3(audioCodec);
  if (mCodecContext == nullptr) {
    logger.error("Audio decoder failed to allocate audio codec context: {}",
                 mPath);
    return -1;
  }

  ret = avcodec_parameters_to_context(mCodecContext, audioStream->codecpar);
  if (ret < 0) {
    logger.error(
        "Audio decoder failed to copy audio codec parameters to context: {}",
        mPath);
    return -1;
  }

  ret = avcodec_open2(mCodecContext, audioCodec, nullptr);
  if (ret < 0) {
    logger.error("Audio decoder failed to open audio codec: {}", mPath);
    return -1;
  }

  mFrame = av_frame_alloc();
  mPacket = av_packet_alloc();

  logger.info("Audio decoder is ready");

  return 0;
}

int AudioDecoder::seek(int64_t timestampUs) {
  if (mFormatContext == nullptr) {
    logger.error("Audio decoder is not initialized");
    return -1;
  }
  int ret = avformat_seek_file(mFormatContext, -1, INT64_MIN, timestampUs,
                               INT64_MAX, 0);
  if (ret < 0) {
    logger.error("Audio decoder failed to seek: {}", mPath);
    return -1;
  } else {
    logger.info("Audio decoder sought to {} us", timestampUs);
  }

  avcodec_flush_buffers(mCodecContext);

  return 0;
}

std::vector<float> AudioDecoder::getNextFrame() {
  if (mFormatContext == nullptr || mCodecContext == nullptr) {
    logger.error("Audio decoder is not initialized");
    return {};
  }

  int ret;
  do {
    while (true) {
      ret = av_read_frame(mFormatContext, mPacket);
      if (mPacket->stream_index == mStreamIndex) {
        break;
      }
    }
    if (ret < 0) {
      if (ret == AVERROR_EOF) {
        logger.info("Audio decoder reached end of file");
        break;
      } else {
        logger.error("Audio decoder failed to read frame: {}", av_err2str(ret));
      }
    }

    ret = avcodec_send_packet(mCodecContext, mPacket);
    if (ret < 0) {
      logger.error("Audio decoder failed to send packet: {}", av_err2str(ret));
      return {};
    }

    ret = avcodec_receive_frame(mCodecContext, mFrame);
    if (ret < 0) {
      if (ret == AVERROR(EAGAIN)) {
        continue;
      }
      logger.error("Audio decoder failed to receive frame: {}",
                   av_err2str(ret));
      return {};
    } else {
      break;
    }
  } while (true);

  if (ret == 0) {
    std::vector<float> samples;
    samples.reserve(mFrame->nb_samples * mFrame->ch_layout.nb_channels);
    interleaveSamples(samples, mFrame);
    return samples;
  }
  return {};
}

AudioParam ted::AudioDecoder::getAudioParam() const {
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
