cmake_minimum_required(VERSION 3.13)
project(TexShadower)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(TS_ROOT ${CMAKE_CURRENT_SOURCE_DIR})

add_executable(TexShadower App/TedShadower.cpp)
target_include_directories(TexShadower PRIVATE ${TS_ROOT})

# add sources
target_sources(TexShadower PRIVATE
    Imgui/imgui.cpp
    Imgui/imgui_draw.cpp
    Imgui/imgui_tables.cpp
    Imgui/imgui_widgets.cpp
    Imgui/imgui_impl_sdl2.cpp
    Imgui/imgui_impl_opengl3.cpp
)

target_sources(TexShadower PRIVATE
    Utils/Utils.cpp
)

target_sources(TexShadower PRIVATE
    Media/VideoDecoder.cpp
    Media/AudioDecoder.cpp
)

# find ffmpeg
find_package(PkgConfig REQUIRED)
pkg_check_modules(FFMPEG REQUIRED IMPORTED_TARGET libavcodec libavformat libavutil)
target_include_directories(TexShadower PRIVATE ${FFMPEG_INCLUDE_DIRS})
target_link_libraries(TexShadower PRIVATE PkgConfig::FFMPEG)

# find opengl3 
find_package(OpenGL REQUIRED)
target_include_directories(TexShadower PRIVATE ${OPENGL_INCLUDE_DIRS})
target_link_libraries(TexShadower PRIVATE ${OPENGL_LIBRARIES})

# find SDL2
find_package(SDL2 REQUIRED)
target_include_directories(TexShadower PRIVATE ${SDL2_INCLUDE_DIRS})
target_link_libraries(TexShadower PRIVATE ${SDL2_LIBRARIES})
