cmake_minimum_required(VERSION 3.5)


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

add_subdirectory( src )