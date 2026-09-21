cmake_minimum_required(VERSION 3.31)
project(fin LANGUAGES CXX)

# Global settings (configure fetched content with)
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

enable_testing()

# FetchContent for gtest
include(FetchContent)
FetchContent_Declare(
    googletest
    GIT_REPOSITORY https://github.com/google/googletest.git
    GIT_TAG v1.17.0
)
FetchContent_MakeAvailable(googletest)

# Global includes
include (GoogleTest)

add_executable(test_OrderPool src/testOrderPool.cpp)
target_link_libraries(test_OrderPool PRIVATE GTest::gtest GTest::gtest_main)

# Add to ctest
gtest_discover_tests(test_OrderPool)
