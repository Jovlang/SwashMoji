cmake_minimum_required(VERSION 3.20)
cmake_policy(SET CMP0091 NEW)
project(SwashMoji LANGUAGES CXX)

include(CTest)

if(NOT WIN32)
  message(FATAL_ERROR "SwashMoji is a Win32 application and must be built on Windows.")
endif()

# Share the self-contained native runtime between application and tests.
set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
add_library(SwashMojiCore STATIC catalog.cpp text.cpp search.cpp personalization.cpp storage.cpp insertion.cpp insertion_win32.cpp)
target_compile_features(SwashMojiCore PUBLIC cxx_std_17)
target_include_directories(SwashMojiCore PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}")
target_compile_definitions(SwashMojiCore PUBLIC UNICODE _UNICODE WIN32_LEAN_AND_MEAN NOMINMAX)
target_link_libraries(SwashMojiCore PUBLIC user32 normaliz)

add_executable(SwashMoji WIN32 main.cpp vocabulary.cpp app.rc)
target_compile_features(SwashMoji PRIVATE cxx_std_17)
target_compile_definitions(SwashMoji PRIVATE UNICODE _UNICODE WIN32_LEAN_AND_MEAN NOMINMAX)
target_link_libraries(SwashMoji PRIVATE SwashMojiCore user32 shell32 uxtheme dwmapi d2d1 dwrite ole32 uiautomationcore)

if(MSVC)
  # Source files are UTF-8; make non-ASCII UI strings compile correctly.
  target_compile_options(SwashMoji PRIVATE /utf-8)
  target_compile_options(SwashMojiCore PUBLIC /utf-8)
endif()

if(BUILD_TESTING)
  add_executable(SwashMojiRankingTests tests/ranking_tests.cpp)
  target_compile_features(SwashMojiRankingTests PRIVATE cxx_std_17)
  target_include_directories(SwashMojiRankingTests PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}")
  add_test(NAME ranking COMMAND SwashMojiRankingTests)
  add_executable(SwashMojiCoreTests tests/core_tests.cpp)
  target_link_libraries(SwashMojiCoreTests PRIVATE SwashMojiCore)
  add_test(NAME core COMMAND SwashMojiCoreTests "${CMAKE_CURRENT_SOURCE_DIR}/emojis.txt")
  add_executable(SwashMojiStorageTests tests/storage_tests.cpp)
  target_link_libraries(SwashMojiStorageTests PRIVATE SwashMojiCore)
  add_test(NAME storage COMMAND SwashMojiStorageTests)
  add_executable(SwashMojiInsertionTests tests/insertion_tests.cpp)
  target_link_libraries(SwashMojiInsertionTests PRIVATE SwashMojiCore)
  add_test(NAME insertion COMMAND SwashMojiInsertionTests)
  add_executable(SwashMojiVocabularyTests tests/vocabulary_tests.cpp)
  target_link_libraries(SwashMojiVocabularyTests PRIVATE SwashMojiCore)
  add_test(NAME vocabulary COMMAND SwashMojiVocabularyTests "${CMAKE_CURRENT_SOURCE_DIR}")
  if(DEFINED ENV{SWASHMOJI_PYTHON})
    set(Python3_EXECUTABLE "$ENV{SWASHMOJI_PYTHON}" CACHE FILEPATH "Python for catalog tests" FORCE)
  endif()
  find_package(Python3 3.10 COMPONENTS Interpreter QUIET)
  if(Python3_Interpreter_FOUND)
    add_test(NAME catalog_generator COMMAND "${Python3_EXECUTABLE}" "${CMAKE_CURRENT_SOURCE_DIR}/tests/test_catalog_generator.py")
  endif()
  add_executable(SwashMojiNativeInputTests tests/native_input_tests.cpp)
  target_link_libraries(SwashMojiNativeInputTests PRIVATE SwashMojiCore)
  add_executable(SwashMojiPickerVocabularyTests tests/picker_vocabulary_tests.cpp vocabulary.cpp app.rc)
  target_link_libraries(SwashMojiPickerVocabularyTests PRIVATE SwashMojiCore shell32 uxtheme dwmapi d2d1 dwrite ole32 uiautomationcore)
  add_executable(SwashMojiVocabularyPreview WIN32 tests/vocabulary_preview.cpp vocabulary.cpp app.rc)
  target_link_libraries(SwashMojiVocabularyPreview PRIVATE SwashMojiCore)
  if(MINGW)
    target_link_options(SwashMojiNativeInputTests PRIVATE -municode)
    target_link_options(SwashMojiPickerVocabularyTests PRIVATE -municode)
    target_link_options(SwashMojiVocabularyPreview PRIVATE -municode)
  endif()
  # Explicit invocation: opens a temporary target window and submits real input.
  # Keep ordinary CTest runs independent of desktop focus and user activity.
  set_property(TARGET SwashMojiRankingTests PROPERTY
    MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
endif()

# Keep the native MSVC build self-contained without a separate VC runtime install.
set_property(TARGET SwashMoji PROPERTY
  MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")

# Keep the searchable emoji catalog next to the executable.
configure_file(emojis.txt emojis.txt COPYONLY)
configure_file(intent_phrases.tsv intent_phrases.tsv COPYONLY)
configure_file(third_party/UNICODE_LICENSE.txt UNICODE_LICENSE.txt COPYONLY)

# MinGW remains supported when used as a native Windows compiler.
if(MINGW)
  target_link_options(SwashMoji PRIVATE -municode -static -static-libgcc -static-libstdc++)
endif()
