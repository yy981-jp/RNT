cmake_minimum_required(VERSION 3.30)
project(
	RNT
	VERSION 0.0.0
	DESCRIPTION "Rename with Texteditor"
	LANGUAGES CXX
)

# cmake_policy(SET CMP0207 NEW)

set(CMAKE_CXX_STANDARD 23)
add_compile_options(-fdiagnostics-color=always)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/build)


# add_subdirectory(external)


set(EXE_NAME "rnt")

file(GLOB_RECURSE SOURCES src/*.cpp)

add_executable(${EXE_NAME} ${SOURCES})

target_include_directories(${EXE_NAME} PRIVATE
	${CMAKE_SOURCE_DIR}/src
)

# External install prefix
if(CMAKE_BUILD_TYPE STREQUAL "Debug")
	list(PREPEND CMAKE_PREFIX_PATH
		${CMAKE_SOURCE_DIR}/external/install/debug
	)
else()
	list(PREPEND CMAKE_PREFIX_PATH
		${CMAKE_SOURCE_DIR}/external/install/release
	)
endif()

find_package(SDL3 CONFIG REQUIRED)

target_link_libraries(${EXE_NAME}
	PRIVATE SDL3::SDL3
)
