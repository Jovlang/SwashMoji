cmake_minimum_required(VERSION 3.20)
cmake_policy(SET CMP0091 NEW)
project(SwashMoji LANGUAGES CXX)

include(CTest)

if(NOT WIN32)
  message(FATAL_ERROR "SwashMoji is a Win32 application and must be built on Windows.")
endif()

add_executable(SwashMoji WIN32 main.cpp app.rc)
target_compile_features(SwashMoji PRIVATE cxx_std_17)
target_compile_definitions(SwashMoji PRIVATE UNICODE _UNICODE WIN32_LEAN_AND_MEAN NOMINMAX)
target_link_libraries(SwashMoji PRIVATE user32 shell32 uxtheme dwmapi d2d1 dwrite ole32 uiautomationcore)

if(MSVC)
  # Source files are UTF-8; make non-ASCII UI strings compile correctly.
  target_compile_options(SwashMoji PRIVATE /utf-8)
endif()

if(BUILD_TESTING)
  add_executable(SwashMojiRankingTests tests/ranking_tests.cpp)
  target_compile_features(SwashMojiRankingTests PRIVATE cxx_std_17)
  target_include_directories(SwashMojiRankingTests PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}")
  add_test(NAME ranking COMMAND SwashMojiRankingTests)
  set_property(TARGET SwashMojiRankingTests PROPERTY
    MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
endif()

# Keep the native MSVC build self-contained without a separate VC runtime install.
set_property(TARGET SwashMoji PROPERTY
  MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")

# Keep the searchable emoji catalog next to the executable.
configure_file(emojis.txt emojis.txt COPYONLY)

# MinGW remains supported when used as a native Windows compiler.
if(MINGW)
  target_link_options(SwashMoji PRIVATE -municode -static -static-libgcc -static-libstdc++)
endif()
