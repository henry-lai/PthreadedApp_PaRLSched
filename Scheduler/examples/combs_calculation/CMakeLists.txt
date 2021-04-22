# ------------------------------- SOURCES ---------------------------------

SET(Combs_Calculation_SRCS
  combs_calculation.cpp
  ThreadRoutine.h
  ThreadRoutine.cpp
)

# ------------------------------- TARGETS --------------------------------

include_directories(${PAPI_INCLUDE_DIRS})
include_directories(${NUMA_INCLUDE_DIRS})
include_directories(${Hwloc_INCLUDE_DIRS})

add_executable(combs_calculation ${Combs_Calculation_SRCS})
if(CMAKE_THREAD_LIBS_INIT)
  target_link_libraries(combs_calculation parlsched "${CMAKE_THREAD_LIBS_INIT}" "${PAPI_LIBRARIES}" "${NUMA_LIBRARY}" "${Hwloc_LIBRARIES}")
endif()


#target_link_libraries( SchedulingExample pthread)

