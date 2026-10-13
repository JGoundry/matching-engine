cmake_minimum_required(VERSION 3.31)
project(
  matching-engine
  VERSION 0.0.1
  DESCRIPTION "Low-latency C++ matching engine"
  HOMEPAGE_URL "https://github.com/jgoundry/matching-engine"
  LANGUAGES CXX
)

include(CTest)

add_subdirectory(src)

if (BUILD_TESTING)
  add_subdirectory(src/test)
endif()
