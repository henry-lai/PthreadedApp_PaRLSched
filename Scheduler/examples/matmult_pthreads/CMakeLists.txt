# ------------------------------- SOURCES ---------------------------------

SET(matmult_pthreads_SRCS
  matmult_pthreads.cpp
)

# ------------------------------- TARGETS --------------------------------

include_directories(${PAPI_INCLUDE_DIRS})
include_directories(${NUMA_INCLUDE_DIRS})
include_directories(${Hwloc_INCLUDE_DIRS})

add_executable(matmult_pthreads ${matmult_pthreads_SRCS})
target_link_libraries( matmult_pthreads pthread)
if(CMAKE_THREAD_LIBS_INIT)
  target_link_libraries(matmult_pthreads parlsched "${CMAKE_THREAD_LIBS_INIT}" "${PAPI_LIBRARIES}" "${NUMA_LIBRARY}" "${Hwloc_LIBRARIES}")
endif()

