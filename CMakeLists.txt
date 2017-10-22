cmake_minimum_required(VERSION 3.7)
project(line-bot-cppcms)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -lcppcms -lcurl")
set(SOURCE_FILES src/index.cpp)
add_executable(app ${SOURCE_FILES})
