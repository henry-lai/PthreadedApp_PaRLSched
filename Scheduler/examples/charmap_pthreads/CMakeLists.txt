# ------------------------------- SOURCES ---------------------------------

SET(character_map_SRCS
  character_map_pthread_sched.cpp)

SET(test_SRCS
  test_program.cpp)

SET(process_SRCS
  process_measurements.cpp)


# ------------------------------- TARGETS --------------------------------

include_directories(${PAPI_INCLUDE_DIRS})
include_directories(${NUMA_INCLUDE_DIRS})
include_directories(${Hwloc_INCLUDE_DIRS})

add_definitions(-DNCO=4)
add_executable(character_map_pthread_sched ${character_map_SRCS})
add_executable(character_map_pthread ${character_map_SRCS})
add_executable(test_program ${test_SRCS})
add_executable(process_measurement ${process_SRCS})
target_compile_definitions(character_map_pthread_sched PUBLIC -DSCHEDULER)
target_link_libraries( character_map_pthread pthread)
if(CMAKE_THREAD_LIBS_INIT)
  target_link_libraries(character_map_pthread_sched parlsched "${CMAKE_THREAD_LIBS_INIT}" "${PAPI_LIBRARIES}" "${NUMA_LIBRARY}" "${Hwloc_LIBRARIES}")
endif()

