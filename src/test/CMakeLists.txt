find_package(GTest CONFIG REQUIRED)

add_executable(test_OrderPool)
target_compile_features(test_OrderPool PRIVATE cxx_std_23)
target_sources(test_OrderPool PRIVATE test_OrderPool.cpp)
target_link_libraries(test_OrderPool PRIVATE matching-engine GTest::gtest GTest::gtest_main)

include(GoogleTest)
gtest_discover_tests(test_OrderPool)

