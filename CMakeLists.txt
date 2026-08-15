cmake_minimum_required(VERSION 3.20)
project(WinMoji LANGUAGES CXX)

if(NOT WIN32)
  message(FATAL_ERROR "WinMoji is a Win32 application and must be built on Windows.")
endif()

add_executable(WinMoji WIN32 main.cpp)
target_compile_features(WinMoji PRIVATE cxx_std_17)
target_compile_definitions(WinMoji PRIVATE UNICODE _UNICODE WIN32_LEAN_AND_MEAN NOMINMAX)
target_link_libraries(WinMoji PRIVATE user32 shell32 uxtheme)

# Keep the searchable emoji catalog next to the executable.
configure_file(emojis.txt emojis.txt COPYONLY)

# MinGW does not infer the wWinMain entry point from _UNICODE as MSVC does.
if(MINGW)
  target_link_options(WinMoji PRIVATE -municode -static -static-libgcc -static-libstdc++)
endif()
