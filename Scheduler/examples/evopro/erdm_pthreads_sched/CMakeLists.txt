project(eRDM_pthreads_sched)
cmake_minimum_required(VERSION 2.8)

set(${PROJECT_NAME}_SOURCES
Config.cpp
ConfigManager.cpp
FIR.cpp
json.cpp
Logger.cpp
LSOptimizer.cpp
main.cpp
mainStuff.cpp
MeasProcessor.cpp
Utils.cpp
RCBProc.cpp
RSBProc.cpp
TaskQueue.cxx
SerialProtocol.cpp
)

add_definitions(-DDEBUG -DNO_BLOCKING )

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O3 -Ofast -flto -Wall -fmessage-length=0 -MMD -MP")

add_executable(${PROJECT_NAME} ${${PROJECT_NAME}_SOURCES})

target_link_libraries(${PROJECT_NAME} m pthread)

add_test(${PROJECT_NAME}_test ${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME} ${TEST_DATA_DIR}/settings_new.json ${TEST_DATA_DIR}/rawSamples_30.dat)

