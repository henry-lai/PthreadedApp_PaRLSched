CMAKE_MINIMUM_REQUIRED(VERSION 2.8)

#-----------------------------------------------------------------------------#
# Configuration section / variables for the STA solution                      #
#-----------------------------------------------------------------------------#

# Project/Solution name
PROJECT(SheetOptimization CXX)

set( STA_BUILD_OPTIFRAMEWORK     YES CACHE BOOL
    "YES: Build the optimization framework" )


set( STA_CONFIG_USE_VLD NO CACHE BOOL
    "YES: #include <vld.h> an enable memory lead debuging using the VLD tool. Effectiv only in debug mode." )


set( STA_DEBUG_MEMORY NO CACHE BOOL
    "YES: memory debugging code is inserted" )

###############################################################################
### !!!   DO NOT MODIFY LINES BELOW FOR ADJUSTING TO YOUR LOCAL NEEDS  !!!  ###
###        All variants should be controlled by proper settings of the      ###    
###                     configuration variable defined above                ###
###############################################################################

#-----------------------------------------------------------------------------#
# Searching for 3rd party libraries                                           #
#-----------------------------------------------------------------------------#

# Boost library search (tested to version 1.53, vc11)

SET(PROJECT_VERSION_MAJOR 0)
SET(PROJECT_VERSION_MINOR 1)
SET(PROJECT_VERSION_PATCH 1)
SET(EXECUTABLE_OUTPUT_PATH "${PROJECT_BINARY_DIR}/bin")
SET(LIBRARY_OUTPUT_PATH "${PROJECT_BINARY_DIR}/bin")

if(UNIX)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -std=c++11")
endif()

set( BOOST_LIBRARYDIR ${STA_CONFIG_BOOST_LIBRARYDIR} )
set( Boost_USE_STATIC_LIBS ON)
set( Boost_USE_MULTITHREADED ON)
if (UNIX)
set( Boost_USE_STATIC_RUNTIME ON)
else()
set( Boost_USE_STATIC_RUNTIME OFF)
endif()
find_package( Boost 1.53.0 REQUIRED COMPONENTS thread date_time filesystem system program_options serialization unit_test_framework random timer chrono)

if(Boost_FOUND)
    include_directories(${Boost_INCLUDE_DIRS})
    link_directories( ${Boost_LIBRARY_DIRS} )
    message( STATUS "Boost C++ ${Boost_MAJOR_VERSION}.${Boost_MINOR_VERSION}" )
    message( STATUS "    IncDirs: ${Boost_INCLUDE_DIRS}" )
    message( STATUS "    LibDirs: ${Boost_LIBRARY_DIRS}" )
    message( STATUS "       Libs: ${Boost_LIBRARIES}" )
endif()


# local 3rd party libs like armadillo, gtest, fastflow, etc. ...

ADD_SUBDIRECTORY(3rd)
include_directories(
    ${ARMADILLO_INCLUDE_DIRS}
    ${FF_INCLUDE_DIRS}
    ${GTESTS_INCLUDE_DIRS}
    ${BLAS_INCLUDE} # before: ${BLAS_DIRECTORIES}
)

message( STATUS "ARMADILLO_INCLUDE_DIRS = ${ARMADILLO_INCLUDE_DIRS}")
message( STATUS "       FF_INCLUDE_DIRS = ${FF_INCLUDE_DIRS}")
message( STATUS "   GTESTS_INCLUDE_DIRS = ${GTESTS_INCLUDE_DIRS}")

# link directories

link_directories(${LIBRARY_OUTPUT_PATH} ${BLAS_INCLUDE})

# Global compiler specific setting


if( MSVC )
    # Avoid warnings about old C functions
    add_definitions( "/D_CRT_SECURE_NO_WARNINGS" )
    
    # Debug Information Format: /Zi = Program Database
	add_definitions( "/Zi" )
    
    # Multi process compilation
	if( STA_CONFIG_MULTI_PROCESSOR_COMPILATION )
        if( STA_CONFIG_MULTI_PROCESSOR_NUMBER GREATER 1 )
            MESSAGE( STATUS "Multi process compilation with /MP${STA_CONFIG_MULTI_PROCESSOR_NUMBER}")
            add_definitions( "/MP${STA_CONFIG_MULTI_PROCESSOR_NUMBER}" )
        else()
            MESSAGE( STATUS "Multi process compilation with /MP")
            add_definitions( "/MP" )
        endif()
	endif()
    
endif()

# In release mod we do not want bounds checking in armadillo
# so we define ARMA_NO_DEBUG 
set( CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -D ARMA_NO_DEBUG" )

message( STATUS "CMAKE_CXX_FLAGS_RELEASE=${CMAKE_CXX_FLAGS_RELEASE}" )

if( STA_DEBUG_MEMORY )
    message( STATUS "Adding -D PROCESS_MEMORY_DEBUGGING" )
    add_definitions( "-D PROCESS_MEMORY_DEBUGGING" )
endif()

if( STA_CONFIG_USE_VLD )
    message( STATUS "Adding -D USE_VLD" )
    add_definitions( "-D USE_VLD" )
endif()

#-----------------------------------------------------------------------------#
# Adding mandatory parts of solution                                          #
#-----------------------------------------------------------------------------#

# add_subdirectory( DataManager       )
# include_directories( ${PROJECT_SOURCE_DIR}/DataManager )

# add_subdirectory( MeasDbDataContext )
# include_directories( ${PROJECT_SOURCE_DIR}/MeasDbDataContext )

#-----------------------------------------------------------------------------#
# Adding "optional" parts of solution                                         #
#-----------------------------------------------------------------------------#

include_directories( 
	${PROJECT_SOURCE_DIR}/OptiFramework/src
    ${PROJECT_SOURCE_DIR}/OptiFramework/src/core
)

if( STA_BUILD_OPTIFRAMEWORK  )
	ADD_SUBDIRECTORY(OptiFramework/src)

endif()

