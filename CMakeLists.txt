cmake_minimum_required (VERSION 3.2)
################################################################################
set(CMAKE_BUILD_TYPE Debug)

################################################################################
project (np1sec-pidgin-plugin)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++14 -Wall -ggdb")

# Use the package PkgConfig to detect GTK+ headers/library files
find_package(PkgConfig REQUIRED)
pkg_check_modules(GTK3 REQUIRED gtk+-3.0)

include_directories(
  "${CMAKE_SOURCE_DIR}/src"
  "${PIDGIN_SRC_DIR}/pidgin"
  "${PIDGIN_SRC_DIR}/libpurple"
  "${GTK3_INCLUDE_DIRS}")

file(GLOB sources
  "${CMAKE_SOURCE_DIR}/src/*.cpp")

add_library(np1sec SHARED ${sources})

################################################################################
