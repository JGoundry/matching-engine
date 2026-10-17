find_package(GTest CONFIG REQUIRED)

add_executable(test_OrderPool)
target_compile_features(test_OrderPool PRIVATE cxx_std_23)
target_sources(test_OrderPool PRIVATE test_OrderPool.cpp)
target_link_libraries(test_OrderPool PRIVATE matching-engine GTest::gmock_main)

add_executable(test_OrderBook)
target_compile_features(test_OrderBook PUBLIC cxx_std_23)
target_sources(test_OrderBook PRIVATE test_OrderBook.cpp)
target_link_libraries(test_OrderBook PRIVATE matching-engine GTest::gmock_main)

include(GoogleTest)
gtest_discover_tests(test_OrderPool)
gtest_discover_tests(test_OrderBook)

