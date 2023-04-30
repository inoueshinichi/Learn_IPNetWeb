cmake_minimum_required(VERSION 3.14.6)

###############################################################################
# Policy
###############################################################################
cmake_policy(SET CMP0115 OLD) # explicit source file extention symbol

# <= CMake3.*.*でAppleClangをコンパイラIDとして認識させる
# Report AppleClang separately from Clang. Their version numbers are different.
# https://cmake.org/cmake/help/v3.0/policy/CMP0025.html
if(POLICY CMP0025)
  cmake_policy(SET CMP0025 NEW)
endif()

###############################################################################
# Device Check
###############################################################################
# Platform
if(WIN32)
    message(STATUS "Platform: Windows ${CMAKE_SYSTEM_VERSION}")
elseif(APPLE)
    message(STATUS "Platform: macOS ${CMAKE_SYSTEM_VERSION}")
elseif(UNIX AND NOT APPLE) # Linux
    message(STATUS "Platform: Linux ${CMAKE_SYSTEM_VERSION}")
else()
    message(FATAL_ERROR "Platform: Not match platforms [Windows, macOS, Linux]. Given platform is ${CMAKE_SYSTEM_NAME}")
endif()

# Processor
message(STATUS "Processor: ${CMAKE_SYSTEM_PROCESSOR}")

# Generator
# https://cmake.org/cmake/help/latest/manual/cmake-generators.7.html
message(STATUS "Generator: ${CMAKE_GENERATOR}")

###############################################################################
# Top-level Project Name
###############################################################################
project(IPNetWeb LANGUAGES C CXX VERSION 0.1.0)

###############################################################################
# Compiler Option for All Target (Easy configration)
###############################################################################
message(STATUS "========== [START] C++ All Target Options ==========")

# ZERO_CHECKプロジェクトの有無
if(NOT BUILD_CPP_ZERO_CHECK)
    set(CMAKE_SUPPRESS_REGENERATION true)
endif()

message(STATUS "========== [END] C++ All Target Options ==========")

###############################################################################
# Test Sample Dirs
###############################################################################

# テスト用ディレクトリ名
set(CMAKE_TEST_DATA_DIRNAME "TestData")

# Generate test_utils.hpp for test data file path
if(WIN32)
    configure_file(test_utils.hpp.tmp test_utils.hpp @ONLY NEWLINE_STYLE CRLF)
else()
    configure_file(test_utils.hpp.tmp test_utils.hpp @ONLY NEWLINE_STYLE LF)
endif()
# include test_utils.hpp
include_directories(${CMAKE_CURRENT_BINARY_DIR})

add_subdirectory(Ping)