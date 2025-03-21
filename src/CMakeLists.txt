
message( STATUS "\n-- ${PROJECT_NAME}: Configuring targets..." )
set( TARGET_OBJ ${PROJECT_NAME}-obj )
set( TARGET_LIB ${PROJECT_NAME}-lib )

################################################################################
# Source files
# ------------------------------------------------------------------------------
# Define the source files for this subdirectory compilation.
# The resulting list of source files is displayed when cmake is invoked.
################################################################################

list( APPEND TARGET_SRCS ${PROJECT_SOURCE_DIR}/src/CalcFodmRegisterValues.cpp )
list( APPEND TARGET_SRCS ${PROJECT_SOURCE_DIR}/src/FirstOrderDelayModel.cpp )

message( STATUS "${PROJECT_NAME}: Defined target source file list..." )
foreach( src ${TARGET_SRCS} )
	message(STATUS "    ${src}")
endforeach()


################################################################################
# Specify the libraries for all targets
################################################################################
link_libraries( 
  ${CONAN_LIBS}
  ${CMAKE_DL_LIBS}
)
message( STATUS "${PROJECT_NAME}: Defined linked library list for src targets..." )

if ( CMAKE_BUILD_TYPE MATCHES Debug )
	enable_testing()
	include(GoogleTest)
	add_subdirectory( test )
endif()

################################################################################
# Configure target object library
# ------------------------------------------------------------------------------
# 
################################################################################
message(STATUS "${PROJECT_NAME}: Creating object library for src target" )
add_library( ${TARGET_OBJ} OBJECT ${TARGET_SRCS} )

target_include_directories( ${TARGET_OBJ}
	PUBLIC
	${CONAN_INCLUDE_DIRS}
	${PROJECT_SOURCE_DIR}/src/
)
message( STATUS "${PROJECT_NAME}: Defined include directory list for src targets..." )
get_property( dirs DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY INCLUDE_DIRECTORIES)
foreach( src ${dirs})
	message(STATUS "    ${src}")
endforeach()

################################################################################
# Configure target static library
# ------------------------------------------------------------------------------
# 
################################################################################

message(STATUS "${PROJECT_NAME}: Creating static library ${TARGET_LIB}" )
add_library( ${TARGET_LIB} STATIC $<TARGET_OBJECTS:${TARGET_OBJ}> )
target_link_libraries(${TARGET_LIB} -lquadmath)

################################################################################
# Configure target properties
################################################################################

set_target_properties( ${TARGET_LIB} ${TARGET_OBJ}
	PROPERTIES 
	COMPILE_FLAGS "${PROJECT_COMPILER_FLAGS}"
	LINK_FLAGS "${PROJECT_LINKER_FLAGS}"
	OUTPUT_NAME ${PROJECT_NAME}
)

foreach(src ${LINKED_LIBS})
	message(STATUS "    ${src}")
endforeach()