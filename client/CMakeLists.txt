cmake_minimum_required(VERSION 3.10)
project(stock_trading_client CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release)
endif()

add_compile_options(-Wall -Wextra)

add_executable(client src/main.cpp)
target_include_directories(client PRIVATE include)
