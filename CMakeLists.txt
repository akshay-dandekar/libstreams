
################################################################################
### Selecting CMake minimum version

CMAKE_MINIMUM_REQUIRED (VERSION 3.4.0)

################################################################################
### Setting Global Parameters

SET(PROJECT_NAME "Streams library")
SET(BINARY_NAME "stream")

PROJECT("${PROJECT_NAME}")

SET(LIBRARY_VERSION_MAJOR 0)
SET(LIBRARY_VERSION_STRING 0.1)

INCLUDE(GNUInstallDirs)

SET(LIBRARY_BASE_PATH "${PROJECT_SOURCE_DIR}")

################################################################################
### Project definition

INCLUDE_DIRECTORIES (
	"${LIBRARY_BASE_PATH}/src"
    "${LIBRARY_BASE_PATH}/src/utils/"
    "${LIBRARY_BASE_PATH}/libs/log/"
)

SET(PUBLIC_SOURCES_FILES
	"${LIBRARY_BASE_PATH}/src/reader.c"
)

SET(PRIVATE_SOURCES_FILES  
    "${LIBRARY_BASE_PATH}/libs/log/log.c"
)

SET(PUBLIC_HEADERS_FILES
	"${LIBRARY_BASE_PATH}/src/reader.h"
    
)

SET(PRIVATE_HEADERS_FILES
    "${LIBRARY_BASE_PATH}/src/utils/conv.h"
    "${LIBRARY_BASE_PATH}/libs/log/log.h"
)

SET(CMAKE_C_VISIBILITY_PRESET hidden)

ADD_LIBRARY (
	${BINARY_NAME} SHARED ${PUBLIC_SOURCES_FILES} ${PRIVATE_SOURCES_FILES}
)

SET_TARGET_PROPERTIES (
	${BINARY_NAME} PROPERTIES
	VERSION		${LIBRARY_VERSION_STRING}
	SOVERSION	${LIBRARY_VERSION_MAJOR}
	PUBLIC_HEADER  ${PUBLIC_HEADERS_FILES}
)

################################################################################
### Adding compilator definitions, like debug symbols

ADD_DEFINITIONS("-g")

################################################################################
### Library installation directives

INSTALL (
	TARGETS ${BINARY_NAME}
	LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
	PUBLIC_HEADER DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)


# Only compile the test library if BUILD_TESTS is ON
if(BUILD_TESTS)

# Link the library to the example program
add_executable(test_reader "${LIBRARY_BASE_PATH}/test/test_reader.c")
target_link_libraries(test_reader PRIVATE stream -lpthread -lserialport)
endif()


################################################################################
# view rawLibrary_CMakeFile.txt hosted with ❤ by GitHub