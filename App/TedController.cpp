#include <fstream>
#include <sys/stat.h>
#include <unistd.h>

#include <SDL_opengl.h>

#include "TedController.h"
#include "Utils/HLS.h"
#include "Utils/ThreadPool.h"

#include "Imgui/imgui.h"
#include "Imgui/imgui_impl_opengl3.h"
#include "Imgui/imgui_impl_sdl2.h"

using ted::logger;

[[maybe_unused]] static std::vector<ted::Subtitle>
retrieveAllSubtitlesFromAss(const std::string &mediaFile) {
  ted::SubtitleDecoder subtitleDecoder(mediaFile);

  int ret;
  ret = subtitleDecoder.init();
  if (ret != 0) {
    return {};
  }

  std::vector<ted::Subtitle> subtitles;
  ted::logger.info("parsing subtitles");
  while (true) {
    ted::Subtitle subtitle;
    ret = subtitleDecoder.getNextSubtitle(subtitle);
    if (ret != 0) {
      break;
    }
    subtitles.push_back(subtitle);
  }

  ted::logger.info("got {} subtitle frames", subtitles.size());

  return subtitles;
}

static const char *CacheDir = "./.cacheMedias";

static inline std::string getCacheFile(const std::string &url) {
  auto file = std::hash<std::string>{}(url);
  std::stringstream ss;
  ss << std::hex;
  ss << CacheDir << "/" << file << ".wav";
  return ss.str();
}

void TedController::GlobalInit() {
  if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER | SDL_INIT_AUDIO |
               SDL_INIT_GAMECONTROLLER) != 0) {
    logger.error("Failed to initialize SDL: {}", SDL_GetError());
    throw std::runtime_error("failed to initialize SDL");
  }

  SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS,
                      SDL_GL_CONTEXT_FORWARD_COMPATIBLE_FLAG);
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 2);
  SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
  SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
  SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 8);
}

TedController::TedController(std::string url)
    : mUrl(std::move(url)), mMediaFile(getCacheFile(mUrl)),
      mThreadPool(ThreadPool(4)) {
  if (access(CacheDir, F_OK) != 0) {
    mkdir(CacheDir, 0777);
  }
  fetchTedTalk();

  mAudioDecoder = std::make_unique<ted::AudioDecoder>(mMediaFile);
  mAudioDecoder->init();
  mPlayer.init(mAudioDecoder->getAudioParam());
  mPlayer.play();

  initUI();
}

int TedController::play() {
  if (mSubtitleIndex >= mSubtitles.size()) {
    logger.error("no more subtitles");
    return -1;
  }

  auto &subtitle = mSubtitles[mSubtitleIndex];
  logger.info("start playing\n {}", subtitle.text);

  int64_t timeDiff =
      std::abs(subtitle.start.us() - mAudioDecoder->getCurrentTime().us());
  if (timeDiff > 1024 / 48000) {
    logger.info("time diff {} is too large, seeking", timeDiff);
    mAudioDecoder->seek(subtitle.start.us());
  }

  while (true) {
    std::shared_ptr<AVFrame> frame;
    int ret = mAudioDecoder->getNextFrame(frame);
    if (ret != 0) {
      logger.error("failed to get next frame");
      return -1;
    }

    if (frame == nullptr) {
      logger.info("no more frames");
      break;
    }

    mPlayer.enqueue(frame);
    if (mAudioDecoder->getCurrentTime() >= subtitle.end) {
      break;
    }
  }

  ++mSubtitleIndex;

  return 0;
}

int TedController::seekByIndex(int64_t index) {
  if (index < 0 || static_cast<size_t>(index) >= mSubtitles.size()) {
    logger.error("invalid index {}", index);
    return -1;
  }

  auto &subtitle = mSubtitles[index];
  logger.info("seeking to subtitle\n {}", subtitle.text);

  mAudioDecoder->seek(subtitle.start.us());

  return 0;
}

void TedController::initUI() {
  auto flags = (SDL_WindowFlags)(SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE |
                                 SDL_WINDOW_ALLOW_HIGHDPI);
  const char *glslVersion = "#version 150";

  mWindow = SDL_CreateWindow("ted", SDL_WINDOWPOS_CENTERED,
                             SDL_WINDOWPOS_CENTERED, 1280, 720, flags);
  mGLContext = SDL_GL_CreateContext(mWindow);
  SDL_GL_MakeCurrent(mWindow, mGLContext);

  SDL_GL_SetSwapInterval(1);

  ImGui::CreateContext();
  ImGuiIO &io = ImGui::GetIO();
  (void)io;
  io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;

  ImGui::StyleColorsDark();

  ImGui_ImplSDL2_InitForOpenGL(mWindow, mGLContext);
  ImGui_ImplOpenGL3_Init(glslVersion);
}

void TedController::run() {
  while (true) {
    SDL_Event event;
    if (SDL_PollEvent(&event)) {
      ImGui_ImplSDL2_ProcessEvent(&event);
      if (event.type == SDL_QUIT) {
        break;
      }
    }

    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplSDL2_NewFrame();
    ImGui::NewFrame();

    {
      ImGui::Begin("ted");
      ImGui::Text("hello, world");
      ImGui::End();
    }

    ImGui::Render();
    auto &io = ImGui::GetIO();
    glViewport(0, 0, (int)io.DisplaySize.x, (int)io.DisplaySize.y);
    glClearColor(0.45f, 0.55f, 0.60f, 1.00f);
    glClear(GL_COLOR_BUFFER_BIT);

    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
    SDL_GL_SwapWindow(mWindow);
  }
}

void TedController::exit() {
  mUserExit.store(true);
  if (mPlayThread.joinable()) {
    mPlayThread.join();
  }
  isRunning = false;
  logger.info("play thread exited");
}

void TedController::runImpl() {
  while (!mUserExit.load()) {
    if (play() != 0) {
      break;
    }
  }
}

void TedController::fetchTedTalk() {
  std::string html;
  ted::SimpleDownloader downloader(mUrl, &html);
  downloader.init();
  downloader.download();

  auto cacheFile = getCacheFile(mUrl);
  std::optional<std::future<void>> audioDownload;
  if (access(cacheFile.c_str(), F_OK) != 0) {
    audioDownload = mThreadPool.enqueue([this, &html]() {
      auto m3u8 = ted::retrieveM3U8UrlFromTalkHtml(html);
      logger.info("fetching media resources from {}", m3u8);

      ted::HLSParser parser(m3u8);
      parser.init();
      if (parser.getAudioPlayList().empty()) {
        throw std::runtime_error("no audio playlist");
      }
      parser.downloadAudioByName("medium", mMediaFile);
    });
  }

  auto subtitleDownload = mThreadPool.enqueue([this, &html]() {
    auto subtitles = ted::retrieveSubtitlesFromTranscript(html);
    mSubtitles = ted::mergeSubtitles(subtitles);
  });

  if (audioDownload) {
    audioDownload->wait();
  }
  subtitleDownload.wait();
}
