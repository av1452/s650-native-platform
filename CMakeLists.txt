cmake_minimum_required(VERSION 3.16)
project(s650_native_platform VERSION 0.1 LANGUAGES CXX)

# C++17, no third-party libraries. The only optional platform code is the macOS window backend (system frameworks).
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
  set(CMAKE_BUILD_TYPE Release CACHE STRING "Build type" FORCE)
endif()

option(S650_BUILD_TESTS "Build the unit tests" ON)
option(S650_WITH_WINDOW "Build the macOS window backend (Cocoa)" ON)

file(GLOB_RECURSE CORE_SOURCES CONFIGURE_DEPENDS src/core/*.cpp src/display/*.cpp src/input/*.cpp src/camera/*.cpp src/storage/*.cpp src/ui/*.cpp)
add_library(s650_core STATIC ${CORE_SOURCES})
target_include_directories(s650_core PUBLIC include)
if(MSVC)
  target_compile_options(s650_core PRIVATE /W4)
else()
  target_compile_options(s650_core PRIVATE -Wall -Wextra -Wpedantic -Wshadow -Wconversion -Wno-sign-conversion)
endif()

add_executable(s650_native src/main.cpp)
target_link_libraries(s650_native PRIVATE s650_core)

if(APPLE AND S650_WITH_WINDOW)
  enable_language(OBJCXX)
  target_sources(s650_native PRIVATE src/display/CocoaWindow.mm)
  target_compile_definitions(s650_native PRIVATE S650_HAVE_WINDOW=1)
  target_compile_options(s650_native PRIVATE -fobjc-arc)
  target_link_libraries(s650_native PRIVATE "-framework Cocoa")
endif()

if(S650_BUILD_TESTS)
  enable_testing()
  add_executable(s650_tests tests/tests_main.cpp tests/test_settings.cpp tests/test_menu.cpp tests/test_playback.cpp
                 tests/test_shooting.cpp tests/test_system.cpp tests/test_render.cpp tests/test_backends.cpp
                 tests/test_display_formats.cpp)
  target_link_libraries(s650_tests PRIVATE s650_core)
  target_include_directories(s650_tests PRIVATE tests)
  add_test(NAME s650_tests COMMAND s650_tests)
endif()
