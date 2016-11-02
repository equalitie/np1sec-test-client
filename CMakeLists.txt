cmake_minimum_required (VERSION 3.2)
################################################################################
set(CMAKE_BUILD_TYPE Debug)
set(GLOB BOOST_VERSION 1.58)

################################################################################
project (np1sec-pidgin-plugin)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++1z -Wall -ggdb")

find_package(Boost ${BOOST_VERSION} COMPONENTS REQUIRED)
# Use the package PkgConfig to detect GTK+ headers/library files
find_package(PkgConfig REQUIRED)
pkg_check_modules(GTK3 REQUIRED gtk+-2.0)

include_directories(
  "${Boost_INCLUDE_DIR}"
  "${CMAKE_SOURCE_DIR}/src"
  "${PIDGIN_SRC_DIR}/pidgin"
  "${PIDGIN_SRC_DIR}/libpurple"
  "${NP1SEC_INCLUDE_DIR}"
  "${GTK3_INCLUDE_DIRS}")

file(GLOB sources
  "${CMAKE_SOURCE_DIR}/src/*.cpp")

add_library(np1sec-plugin SHARED ${sources})
target_link_libraries(np1sec-plugin ${NP1SEC_LIB_DIR}/libnp1sec.so)

################################################################################
