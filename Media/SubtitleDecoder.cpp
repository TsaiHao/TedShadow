#include "MediaSubtitleDecoder.h"
#include "Utils/Utils.h"
#include <regex>

using ted::Subtitle;

int Subtitle::merge(const Subtitle &next) {
  if (next.start.us() > end.us()) {
    return -1;
  }

  if (next.start.us() < end.us()) {
    text += "\n";
  }

  text += next.text;
  end = next.end;

  return 0;
}

using ted::MediaSubtitleDecoder;

MediaSubtitleDecoder::MediaSubtitleDecoder(std::string mediaFile)
    : DecoderBase(std::move(mediaFile), AVMEDIA_TYPE_SUBTITLE) {}

MediaSubtitleDecoder::~MediaSubtitleDecoder() = default;

int MediaSubtitleDecoder::init() {
  int ret = DecoderBase::init();

  return ret;
}

int MediaSubtitleDecoder::seek(int64_t timestampUs) {
  return DecoderBase::seek(timestampUs);
}

int MediaSubtitleDecoder::getNextFrame(std::shared_ptr<AVFrame> &frame) {
  int ret = decodeLoopOnce();
  if (ret != 0) {
    return ret;
  }

  frame = nullptr;
  return 0;
}

static std::string parseAssDialogue(const char *ass) {
  std::string res;
  int sepPos = 0;
  int nComma = 0;
  auto len = (int)strlen(ass);

  for (int i = 0; i < len; ++i) {
    if (ass[i] == ',') {
      ++nComma;
      if (nComma == 8) {
        sepPos = i;
        break;
      }
    }
  }

  if (nComma == 8) {
    res = std::string(&ass[sepPos + 1], len - sepPos - 1);
  }

  return res;
}

int MediaSubtitleDecoder::getNextSubtitle(Subtitle &subtitle) {
  int ret;
  int gotSub = 1;
  int64_t pts = 0, duration = 0;

  AVSubtitle avSub;
  do {
    do {
      ret = av_read_frame(mFormatContext, mPacket);
      if (ret < 0) {
        logger.error("Subtitle read frame failed: {}", getFFmpegErrorStr(ret));
        return ret;
      }
      if (mPacket->stream_index == mStreamIndex) {
        break;
      }
    } while (true);

    ret = avcodec_decode_subtitle2(mCodecContext, &avSub, &gotSub, mPacket);
    if (ret < 0) {
      logger.error("Subtitle decode failed: {}", getFFmpegErrorStr(ret));
      return ret;
    }
    if (gotSub == 0) {
      continue;
    }

    pts = mPacket->pts;
    duration = mPacket->duration;
  } while (gotSub == 0);

  if (avSub.num_rects != 1) {
    logger.error("Subtitle decode failed: invalid subtitle rects {}",
                 avSub.num_rects);
    return -1;
  }

  subtitle.text = parseAssDialogue(avSub.rects[0]->ass);
  subtitle.start = Time::fromUs(pts);
  subtitle.end = Time::fromUs(pts + duration);

  avsubtitle_free(&avSub);

  return 0;
}

std::vector<Subtitle>
ted::retrieveSubtitlesFromTranscript(const std::string &html) {
  static std::regex pattern(
      R"(<script id="__NEXT_DATA__" type="application/json">(.*?)</script>)");

  std::smatch match;
  std::regex_search(html, match, pattern);
  if (match.size() != 2) {
    throw std::runtime_error("regex search failed");
  }

  auto json = nlohmann::json::parse(match[1].str());
  auto paragraphs =
      json["props"]["pageProps"]["transcriptData"]["translation"]["paragraphs"];
  assert(paragraphs.is_array());

  logger.info("Html subtitle parsed, {} paragraphs got", paragraphs.size());

  std::vector<Subtitle> subtitles;

  for (auto &&para : paragraphs) {
    if (para["__typename"] != "Paragraph") {
      continue;
    }
    auto &cues = para["cues"];
    for (auto &&cue : cues) {
      if (cue["__typename"] != "Cue") {
        continue;
      }

      auto text = cue["text"].get<std::string>();
      auto timestamp = cue["time"].get<int64_t>();
      if (!subtitles.empty()) {
        subtitles.back().end = Time::fromMs(timestamp);
      }

      if (text.empty() || text.starts_with('(') || timestamp < 0) {
        continue;
      }

      replaceAll(text, "\\n", ", ");
      subtitles.emplace_back(Subtitle{
          .text = std::move(text),
          .start = Time::fromMs(timestamp),
      });
    }
  }

  assert(!subtitles.empty() && "no subtitles found");
  if (subtitles.back().end == Time::fromMs(0)) {
    auto duration = json["props"]["pageProps"]["videoData"]["duration"];
    subtitles.back().end = Time::fromS(duration.get<int>());
  }

  return subtitles;
}