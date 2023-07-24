#include "DecoderBase.h"
#include "Utils/Utils.h"

using ted::DecoderBase;

DecoderBase::DecoderBase(std::string mediaFile, AVMediaType type)
    : mPath(std::move(mediaFile)), mMediaType(type) {}

DecoderBase::~DecoderBase() {
  if (mCodecContext != nullptr) {
    avcodec_free_context(&mCodecContext);
  }
  if (mFormatContext != nullptr) {
    avformat_close_input(&mFormatContext);
  }
  if (mFrame != nullptr) {
    av_frame_free(&mFrame);
  }
  if (mPacket != nullptr) {
    av_packet_free(&mPacket);
  }
}

int DecoderBase::init() {
  int ret;
  ret = avformat_open_input(&mFormatContext, mPath.c_str(), nullptr, nullptr);
  if (ret < 0) {
    logger.error("Decoder failed to open input file: {}, {}", mPath, TYPE_STR);
    return -1;
  }

  ret = avformat_find_stream_info(mFormatContext, nullptr);
  if (ret < 0) {
    logger.error("Decoder failed to find stream info: {}, {}", mPath, TYPE_STR);
    return -1;
  }

  int streamIndex = av_find_best_stream(mFormatContext, mMediaType, -1,
                                        -1, nullptr, 0);
  if (streamIndex < 0) {
    logger.error("Decoder failed to find audio stream: {}, {}", mPath,
                 TYPE_STR);
    return -1;
  }
  mStreamIndex = streamIndex;

  for (int i = 0; i < (int)mFormatContext->nb_streams; ++i) {
    if (i == mStreamIndex) {
      continue;
    }
    mFormatContext->streams[i]->discard = AVDISCARD_ALL;
  }

  AVStream *stream = mFormatContext->streams[mStreamIndex];
  AVCodec const *audioCodec =
      avcodec_find_decoder(stream->codecpar->codec_id);
  if (audioCodec == nullptr) {
    logger.error("Decoder failed to find decoder: {}, {}", mPath,
                 TYPE_STR);
    return -1;
  }

  mCodecContext = avcodec_alloc_context3(audioCodec);
  if (mCodecContext == nullptr) {
    logger.error("Decoder failed to allocate audio codec context: {}, {}",
                 mPath, TYPE_STR);
    return -1;
  }

  ret = avcodec_parameters_to_context(mCodecContext, stream->codecpar);
  if (ret < 0) {
    logger.error(
        "Decoder failed to copy audio codec parameters to context: {}, {}",
        mPath, TYPE_STR);
    return -1;
  }

  ret = avcodec_open2(mCodecContext, audioCodec, nullptr);
  if (ret < 0) {
    logger.error("Decoder failed to open audio codec: {}, {}, {}", mPath,
                 TYPE_STR, getFFmpegErrorStr(ret));
    return -1;
  }
  logger.info("Decoder opened, codec {}, stream {}, {}", audioCodec->name,
              mStreamIndex, TYPE_STR);

  mFrame = av_frame_alloc();
  mPacket = av_packet_alloc();
  return 0;
}

int DecoderBase::seek(int64_t timestampUs) {
  if (mFormatContext == nullptr) {
    logger.error("Decoder is not initialized, {}", TYPE_STR);
    return -1;
  }
  int ret = avformat_seek_file(mFormatContext, -1, INT64_MIN, timestampUs,
                               INT64_MAX, 0);
  if (ret < 0) {
    logger.error("Decoder failed to seek: {}, {}", mPath, TYPE_STR);
    return -1;
  } else {
    logger.info("Decoder sought to {} us, {}", timestampUs, TYPE_STR);
  }

  avcodec_flush_buffers(mCodecContext);
  return 0;
}

int DecoderBase::decodeLoopOnce() {
  if (mFormatContext == nullptr || mCodecContext == nullptr) {
    logger.error("Audio decoder is not initialized");
    return -1;
  }

  int ret;
  do {
    while (true) {
      ret = av_read_frame(mFormatContext, mPacket);
      if (mPacket->stream_index == mStreamIndex || ret < 0) {
        break;
      }
    }
    if (ret < 0) {
      if (ret == AVERROR_EOF) {
        logger.info("Decoder reached end of file, {}", TYPE_STR);
        break;
      } else {
        logger.error("Decoder failed to read frame: {}, {}",
                     getFFmpegErrorStr(ret), TYPE_STR);
        return ret;
      }
    }

    ret = avcodec_send_packet(mCodecContext, mPacket);
    if (ret < 0) {
      logger.error("Decoder failed to send packet: {}, {}",
                   getFFmpegErrorStr(ret), TYPE_STR);
      return ret;
    }

    ret = avcodec_receive_frame(mCodecContext, mFrame);
    if (ret < 0) {
      if (ret == AVERROR(EAGAIN)) {
        continue;
      }
      logger.error("Decoder failed to receive frame: {}, {}",
                   getFFmpegErrorStr(ret), TYPE_STR);
      return ret;
    } else {
      break;
    }
  } while (true);

  return 0;
}

