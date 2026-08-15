cmake_minimum_required(VERSION 3.20)
cmake_policy(SET CMP0091 NEW)
project(EmoPicker LANGUAGES CXX)

if(NOT WIN32)
  message(FATAL_ERROR "EmoPicker is a Win32 application and must be built on Windows.")
endif()

add_executable(EmoPicker WIN32 main.cpp app.rc)
target_compile_features(EmoPicker PRIVATE cxx_std_17)
target_compile_definitions(EmoPicker PRIVATE UNICODE _UNICODE WIN32_LEAN_AND_MEAN NOMINMAX)
target_link_libraries(EmoPicker PRIVATE user32 shell32 uxtheme d2d1 dwrite)

# Keep the native MSVC build self-contained without a separate VC runtime install.
set_property(TARGET EmoPicker PROPERTY
  MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")

# Keep the searchable emoji catalog next to the executable.
configure_file(emojis.txt emojis.txt COPYONLY)

# MinGW remains supported when used as a native Windows compiler.
if(MINGW)
  target_link_options(EmoPicker PRIVATE -municode -static -static-libgcc -static-libstdc++)
endif()
