cmake_minimum_required(VERSION 3.20)
option(use_conan "Use Conan" true)

################################################################################
# Extract the version information for this library
# ------------------------------------------------------------------------------
# This information should be stored in a text file named .release at the
# top-level of the project. In order to convey the release version, the 
# file should contain the line:
#
# release=(0-9).(0-9).(0-9)
#
################################################################################
file( READ ".release" rel )
string( REGEX MATCH "name=([A-Za-z0-9\-]*)" _ ${rel} )
set( LIB_NAME ${CMAKE_MATCH_1} )

string( REGEX MATCH "release=([0-9.]*)" _ ${rel} )
set( RELEASE ${CMAKE_MATCH_1} )

string( REGEX MATCH "description=(.*)" _ ${rel} )
set( PROJECT_DESCRIPTION ${CMAKE_MATCH_1} )

get_filename_component( PACKAGE_NAME ${CMAKE_CURRENT_LIST_DIR} NAME )
project( ${PACKAGE_NAME} VERSION ${RELEASE} )

set( CMAKE_EXPORT_COMPILE_COMMANDS "ON" )
message( STATUS "${PROJECT_NAME}: Exporting compile commands..." )

if (use_conan)
  include(${CMAKE_BINARY_DIR}/conanbuildinfo.cmake)
  conan_basic_setup()
endif()

###############################################################################
# Set testing flags if required
# ------------------------------------------------------------------------------
# Configure test flags for the compiler and the linker.
# Default is no special test-related flags.
# These symbols are passed to the subdirectories that are compiled within the
# project and applied as required in order to facilitate testing and coverage
# requirements.
################################################################################

message(STATUS "${CMAKE_PROJECT_NAME}: Setting test compiler and linker property flags..." )
if ( CMAKE_BUILD_TYPE MATCHES Debug )
  set( PROJECT_LINKER_FLAGS "-pthread --coverage -fprofile-arcs" )
  set( PROJECT_COMPILER_FLAGS "-g -fprofile-arcs -ftest-coverage -fkeep-inline-functions" )
else()
  set( PROJECT_LINKER_FLAGS "-pthread -Wl,--strip-all" )
  set( PROJECT_COMPILER_FLAGS  )
endif( CMAKE_BUILD_TYPE MATCHES Debug )

# GoogleTest requires at least C++14
set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

################################################################################
# Add include paths to Project
# ------------------------------------------------------------------------------
# Add subdirectories to the project by including them here. Each subdirectory
# must have a CMakeLists.txt file included.
################################################################################

add_subdirectory( src )