# ------------------------------- SOURCES ---------------------------------

SET(fluidanimate_SRCS
  parsec_barrier.cpp cellpool.cpp pthreads_scheduler.cpp
)

SET(fluidanimate_seq_SRCS
  cellpool.cpp serial.cpp
)

SET(fluidanimate_nosched_SRCS
  parsec_barrier.cpp cellpool.cpp pthreads.cpp
)

# ------------------------------- TARGETS --------------------------------

include_directories(${PAPI_INCLUDE_DIRS})
include_directories(${NUMA_INCLUDE_DIRS})
include_directories(${Hwloc_INCLUDE_DIRS})

add_executable(fluidanimate ${fluidanimate_SRCS})
add_executable(fluidanimate_seq ${fluidanimate_seq_SRCS})
add_executable(fluidanimate_nosched ${fluidanimate_nosched_SRCS})
target_link_libraries( fluidanimate pthread)
target_link_libraries( fluidanimate_nosched pthread)
if(CMAKE_THREAD_LIBS_INIT)
  target_link_libraries(fluidanimate parlsched "${CMAKE_THREAD_LIBS_INIT}" "${PAPI_LIBRARIES}" "${NUMA_LIBRARY}" "${Hwloc_LIBRARIES}")
endif()

