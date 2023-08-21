#pragma once

#include <sstream>
#include <vector>

#include "Media/AudioDecoder.h"
#include "Media/AudioPlayer.h"
#include "Media/SubtitleDecoder.h"
#include "Utils/Utils.h"
#include "Utils/ThreadPool.h"

class TedController {
public:
  static void GlobalInit();

  explicit TedController(std::string url);

  void run();

  void exit();

private:
  void initUI();

  int play();

  void runImpl();

  int seekByIndex(int64_t index);

  void fetchTedTalk();

  std::atomic<bool> mUserExit = false;

  std::string mUrl;
  std::string mMediaFile;
  std::string mSubtitleFile;

  ted::AudioPlayer mPlayer;
  std::unique_ptr<ted::AudioDecoder> mAudioDecoder;

  std::vector<ted::Subtitle> mSubtitles;
  decltype(mSubtitles)::size_type mSubtitleIndex = 0;

  SDL_GLContext mGLContext;
  SDL_Window* mWindow;

  ThreadPool mThreadPool;
  std::thread mPlayThread;
  std::atomic<bool> isRunning = false;
};
