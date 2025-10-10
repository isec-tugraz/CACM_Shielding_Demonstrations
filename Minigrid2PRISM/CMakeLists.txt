include(util/CMakeLists.txt)

set(CMAKE_CXX_STANDARD 20)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -D__FILENAME__='\"$(subst ${CMAKE_SOURCE_DIR}/,,$(abspath $<))\"'")
add_definitions(-DLOG_DEBUG)

cmake_minimum_required(VERSION 3.0...3.22)

set(CMAKE_BUILD_TYPE Debug)

project(
  Minigrid2PRISM
  VERSION 0.1
  LANGUAGES CXX)

find_package(yaml-cpp)

add_executable(main
               ${SRCS}
               main.cpp
               )

target_link_libraries(main pthread yaml-cpp::yaml-cpp)

