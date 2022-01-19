cmake_minimum_required(VERSION 3.5)
project(agile-workflows VERSION 0.0.1 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include(${CMAKE_BINARY_DIR}/conanbuildinfo.cmake)
conan_basic_setup(TARGETS KEEP_RPATHS)

set(CMAKE_EXPORT_COMPILE_COMMANDS 1)

if (NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
  message(STATUS "No build type selected, default to Debug")
  set(CMAKE_BUILD_TYPE "Debug" CACHE STRING "Build type (default Debug)" FORCE)
endif()


# No in-tree build allowed.
if (CMAKE_SOURCE_DIR STREQUAL CMAKE_BINARY_DIR)
  message(FATAL_ERROR
    "In-source build are not allowed.
Please create a directory directory and run cmake from there, passing the path
to this source directory as the last argumente.
This process created the file `CMakeCache.txt' and the directory `CMakeFiles'.
Please delete them.")
endif()


string(TOUPPER "${CMAKE_BUILD_TYPE}" uppercase_CMAKE_BUILD_TYPE)

if (CMAKE_BUILD_TYPE AND
    NOT uppercase_CMAKE_BUILD_TYPE MATCHES "^(DEBUG|RELEASE|RELWITHDEBINFO|MINSIZEREL)$")
  message(FATAL_ERROR "Invalid value for CMAKE_BUILD_TYPE: ${CMAKE_BUILD_TYPE}")
endif()

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include)

set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -O3")
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -g")
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -D_GLIBCXX_DEBUG -D_GLIBCXX_DEBUG_PEDANTIC")
set(CMAKE_CXX_FLAGS_RELWITHDEBINFO "${CMAKE_CXX_FLAGS_RELEASE} ${CMAKE_CXX_FLAGS_DEBUG}")

find_package(MPI REQUIRED)
find_package(Threads REQUIRED)

add_subdirectory(tools)

enable_testing()
include(CTest)
