set( PARLSCHED_SRCS
		Scheduler.h
		Scheduler.cpp
		ThreadControl.h
		ThreadControl.cpp
		ThreadInfo.h
		ThreadSuspendControl.h
)

# -------------------------------- TARGETS --------------------------------

include_directories(${PAPI_INCLUDE_DIRS})
include_directories(${NUMA_INCLUDE_DIRS})

# parlsched library target
ADD_LIBRARY(parlsched STATIC ${PARLSCHED_SRCS})

#TARGET_LINK_LIBRARIES(parlsched "${CMAKE_THREAD_LIBS_INIT}" "${PAPI_LIBRARIES}")