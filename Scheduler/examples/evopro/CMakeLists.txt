project(eRDM)
cmake_minimum_required(VERSION 2.8)

set(CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/cmake/modules)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++0x")

SET(CMAKE_SHARED_LIBRARY_LINK_CXX_FLAGS)

include_directories(fastflow)

option(BUILD_CPU "Build eRDM CPU parallel version" OFF)

option(BUILD_GPU "Build eRDM GPU OpenCL version" OFF)

option(BUILD_OFFLINE "Build eRDM sequential version" ON)

option(BUILD_SCHEDULER "Build eRDM scheduler version" ON)

option(BUILD_TESTS "Build tests" ON)

set(TEST_DATA_DIR "${CMAKE_CURRENT_SOURCE_DIR}/data" CACHE PATH "Path to test data directory")

if(BUILD_TESTS)
  enable_testing()
endif()

if(BUILD_CPU)
  add_subdirectory(erdm_cpu)
endif()

if(BUILD_GPU)
  add_subdirectory(erdm_gpu)
endif()


if(BUILD_OFFLINE)
  add_subdirectory(erdm_offline)
endif()

if(BUILD_SCHEDULER)
  add_subdirectory(erdm_pthreads_sched)
endif()
