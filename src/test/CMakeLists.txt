################################################################################
# Target Name
# ------------------------------------------------------------------------------
# Set the target name for this subdirectory here.
################################################################################

message( STATUS "\n-- ${PROJECT_NAME}: Configuring test targets..." )
set( TEST_TARGET_OBJ ${PROJECT_NAME}-test-obj )
set( TEST_TARGET_BIN test-${PROJECT_NAME} )
set( TEST_TARGET_LIB ${PROJECT_NAME}-test-lib )
set( TEST_SOURCE_DIR ${PROJECT_SOURCE_DIR}/src/test )
set( TEST_WORKDIR "${CMAKE_BINARY_DIR}/src/test" )

message( STATUS "\n-- ${PROJECT_NAME}: Test working directory is ${TEST_WORKDIR}" )

include(CTest)

################################################################################
# Source files
# ------------------------------------------------------------------------------
# Define the source files for this subdirectory compilation.
# The resulting list of source files is displayed when cmake is invoked.
################################################################################

list( APPEND TEST_TARGET_SRCS ${TEST_SOURCE_DIR}/test_CalcFODMRegValues.cpp )
message( STATUS "${PROJECT_NAME}: Defined test source file list..." )
foreach( src ${TEST_TARGET_SRCS} )
	message(STATUS "    ${src}")
endforeach()

################################################################################
# Specify the libraries for all targets
################################################################################

link_libraries(
	${TARGET_LIB}
  )
message( STATUS "${PROJECT_NAME}: Defined linked library list for test targets..." )


################################################################################
# Configure test target object library
# ------------------------------------------------------------------------------
# 
################################################################################

message(STATUS "${PROJECT_NAME}: Creating object library for test target" )
add_library( ${TEST_TARGET_OBJ} OBJECT ${TEST_TARGET_SRCS} )

target_include_directories( ${TEST_TARGET_OBJ}
	PUBLIC
	${CONAN_INCLUDE_DIRS}
	${PROJECT_SOURCE_DIR}/src
	${PROJECT_SOURCE_DIR}/src/test/
	)
message( STATUS "${PROJECT_NAME}: Defined include directory list for test targets..." )
get_property( dirs DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY INCLUDE_DIRECTORIES)
foreach( src ${dirs})
	message(STATUS "    ${src}")
endforeach()

################################################################################
# Configure target test executable
# ------------------------------------------------------------------------------
# 
################################################################################

message(STATUS "${PROJECT_NAME}: Creating test executable ${TEST_TARGET_BIN}" )
add_executable( ${TEST_TARGET_BIN} $<TARGET_OBJECTS:${TEST_TARGET_OBJ}> )

add_test( NAME ${PROJECT_NAME}_test
	COMMAND ${CMAKE_BINARY_DIR}/bin/${TEST_TARGET_BIN} --gtest_output=xml:${CMAKE_BINARY_DIR}/report.xml 
	WORKING_DIRECTORY ${TEST_WORKDIR} 
)

################################################################################
# Specify instrumented test library
################################################################################

message( STATUS "\n-- ${PROJECT_NAME}: Creating test library ${TEST_TARGET_LIB}" )
add_library( ${TEST_TARGET_LIB} STATIC $<TARGET_OBJECTS:${TEST_TARGET_OBJ}> )

set_target_properties( ${TEST_TARGET_BIN} ${TEST_TARGET_LIB} ${TEST_TARGET_OBJ}
	PROPERTIES 
	COMPILE_FLAGS "${PROJECT_COMPILER_FLAGS}"
	LINK_FLAGS "${PROJECT_LINKER_FLAGS}"
	OUTPUT_NAME test-${PROJECT_NAME}
	RUNTIME_OUTPUT_DIRECTORY ${TEST_WORKDIR}
)

# Link test executable against gtest & gtest_main
target_link_libraries(${TEST_TARGET_RUN} ${TEST_TARGET_BIN})

message(STATUS "${PROJECT_NAME}: Linked libraries for test targets" )
get_target_property( LINKED_LIBS ${TEST_TARGET_BIN} LINK_LIBRARIES )
foreach(src ${LINKED_LIBS})
	message(STATUS "    ${src}")
endforeach()

################################################################################
# Copy files to test working directory
################################################################################

configure_file(${PROJECT_SOURCE_DIR}/python/fodm_calc_ref.py ${TEST_WORKDIR}/. COPYONLY)
configure_file(${PROJECT_SOURCE_DIR}/src/test/fodm_test_input.csv ${TEST_WORKDIR}/. COPYONLY)

#add_test(NAME TestFODMGen COMMAND ${TEST_TARGET_BIN} WORKING_DIRECTORY ${TEST_WORKDIR})
