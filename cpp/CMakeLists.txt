cmake_minimum_required(VERSION 3.20)

# gtest w/o conan
include(FetchContent)
FetchContent_Declare(
  googletest
  URL https://github.com/google/googletest/archive/refs/tags/v1.16.0.zip
)
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)
message(STATUS "GTest fetched.")

file( READ "../.release" rel )
string( REGEX MATCH "release=([0-9.]*)" _ ${rel} )
set( RELEASE ${CMAKE_MATCH_1} )

project( FirstOrderDelayModel VERSION ${RELEASE} )
string( TOLOWER ${PROJECT_NAME} PROJECT_LIB_NAME )
set( TARGET_OBJ ${PROJECT_LIB_NAME}-obj )
set( TARGET_LIB ${PROJECT_LIB_NAME}-lib )
list( APPEND TARGET_SRCS ${PROJECT_SOURCE_DIR}/FirstOrderDelayModel.cpp )

enable_testing()
include(GoogleTest)
add_subdirectory(tests)
message(STATUS "Testing enabled.")

add_library( ${TARGET_OBJ} OBJECT ${TARGET_SRCS} )

target_include_directories( ${TARGET_OBJ}
	PUBLIC
	${PROJECT_SOURCE_DIR}
)

add_library( ${TARGET_LIB} STATIC $<TARGET_OBJECTS:${TARGET_OBJ}> )

set_target_properties( ${TARGET_LIB} ${TARGET_OBJ}
	PROPERTIES 
	COMPILE_FLAGS "${PROJECT_COMPILER_FLAGS}"
	LINK_FLAGS "${PROJECT_LINKER_FLAGS}"
	OUTPUT_NAME ${PROJECT_LIB_NAME}
)
