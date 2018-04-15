cmake_minimum_required(VERSION 3.11)
project(line-bot-cppcms)

find_library(CPPCMS cppcms)
find_library(BOOSTER booster)
find_library(CURL curl)
add_definitions(-Wall)

set(CMAKE_CXX_STANDARD 11)
set(SOURCE_FILES src/index.cpp)
add_executable(app ${SOURCE_FILES})
target_link_libraries(app ${CPPCMS} ${BOOSTER} ${CURL})
