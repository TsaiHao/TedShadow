cmake_minimum_required(VERSION 3.13)
project(TedShadow)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_BUILD_TYPE Debug)

set(PKG_CONFIG_PATH ${PKG_CONFIG_PATH} /usr/lib64/pkgconfig)
set(TS_ROOT ${CMAKE_CURRENT_SOURCE_DIR})

option(TS_ENABLE_TEST "Enable test" OFF)

add_executable(TedShadow App/TedShadow.cpp)
target_include_directories(TedShadow PRIVATE ${TS_ROOT})
target_compile_options(TedShadow PRIVATE -Wall -Wextra -Werror)

# add sources
set(IMGUI_SOURCES
    Imgui/imgui.cpp
    Imgui/imgui_draw.cpp
    Imgui/imgui_tables.cpp
    Imgui/imgui_widgets.cpp
    Imgui/imgui_impl_sdl2.cpp
    Imgui/imgui_impl_opengl3.cpp
)
target_sources(TedShadow PRIVATE
    ${IMGUI_SOURCES}
)

set(UTILS_SOURCES Utils/Utils.cpp)
target_sources(TedShadow PRIVATE
    ${UTILS_SOURCES}
)

set(MEDIA_SOURCES
    Media/DecoderBase.cpp
    Media/VideoDecoder.cpp
    Media/AudioDecoder.cpp
    Media/AudioPlayer.cpp
    Media/GLRenderer.cpp
    Media/SubtitleDecoder.cpp
)
target_sources(TedShadow PRIVATE
    ${MEDIA_SOURCES}
)

set(APP_SOURCES
    App/TedShadow.cpp
)
target_sources(TedShadow PRIVATE
    ${APP_SOURCES}
)

# find ffmpeg
find_package(PkgConfig REQUIRED)
pkg_check_modules(FFMPEG REQUIRED IMPORTED_TARGET libavcodec libavformat libavutil)
target_include_directories(TedShadow PRIVATE ${FFMPEG_INCLUDE_DIRS})
target_link_libraries(TedShadow PRIVATE PkgConfig::FFMPEG)

# find opengl3 
find_package(OpenGL REQUIRED)
target_include_directories(TedShadow PRIVATE ${OPENGL_INCLUDE_DIRS})
target_link_libraries(TedShadow PRIVATE ${OPENGL_LIBRARIES})

# find SDL2
find_package(SDL2 REQUIRED)
target_include_directories(TedShadow PRIVATE ${SDL2_INCLUDE_DIRS})
target_link_libraries(TedShadow PRIVATE ${SDL2_LIBRARIES})

# find fmt
pkg_check_modules(FMT REQUIRED IMPORTED_TARGET fmt)
target_link_libraries(TedShadow PRIVATE PkgConfig::FMT)
target_include_directories(TedShadow PRIVATE ${FMT_INCLUDE_DIRS})

# find curl
find_package(CURL REQUIRED)
target_include_directories(TedShadow PRIVATE ${CURL_INCLUDE_DIRS})
target_link_libraries(TedShadow PRIVATE ${CURL_LIBRARIES})

if (TS_ENABLE_TEST)
    Include(FetchContent)
    FetchContent_Declare(
        Catch2
        GIT_REPOSITORY https://github.com/catchorg/Catch2.git
        GIT_TAG v3.4.0
    )

    FetchContent_MakeAvailable(Catch2)

    add_executable(MediaTest Media/MediaTest.cpp)
    target_link_libraries(MediaTest PRIVATE
            Catch2::Catch2WithMain
            CURL::libcurl
            PkgConfig::FMT
            PkgConfig::FFMPEG
            SDL2::SDL2
            )
    target_include_directories(MediaTest PRIVATE ${TS_ROOT})
    target_sources(MediaTest PRIVATE ${MEDIA_SOURCES} ${UTILS_SOURCES})
endif()