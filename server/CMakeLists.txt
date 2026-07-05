cmake_minimum_required(VERSION 3.10)
project(stock_trading_server CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release)
endif()

add_compile_options(-Wall -Wextra)

find_package(Threads REQUIRED)

set(SERVER_LIB_SOURCES
    src/stock.cpp
    src/order.cpp
    src/orderbook.cpp
    src/trader.cpp
    src/market.cpp
)

add_library(server_core STATIC ${SERVER_LIB_SOURCES})
target_include_directories(server_core PUBLIC include)
target_link_libraries(server_core PUBLIC Threads::Threads)

add_executable(server src/main.cpp)
target_include_directories(server PRIVATE include)
target_link_libraries(server PRIVATE server_core Threads::Threads)

option(BUILD_SERVER_TESTS "Build GoogleTest unit tests for the stock server" ON)

if(BUILD_SERVER_TESTS)
  include(FetchContent)
  if(POLICY CMP0135)
    cmake_policy(SET CMP0135 NEW)
  endif()
  FetchContent_Declare(
    googletest
    URL https://github.com/google/googletest/archive/refs/tags/v1.15.2.zip
  )
  set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
  FetchContent_MakeAvailable(googletest)

  enable_testing()

  set(TEST_SOURCES
      tests/test_stock.cpp
      tests/test_order.cpp
      tests/test_orderbook.cpp
      tests/test_trader.cpp
      tests/test_market.cpp
  )
  add_executable(server_tests ${TEST_SOURCES})
  target_include_directories(server_tests PRIVATE include)
  target_link_libraries(server_tests PRIVATE server_core GTest::gtest_main)

  include(GoogleTest)
  gtest_discover_tests(server_tests)
endif()
