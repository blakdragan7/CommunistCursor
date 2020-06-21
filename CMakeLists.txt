cmake_minimum_required(VERSION 3.0.0)

set_property(GLOBAL PROPERTY USE_FOLDERS ON)

project(CommunistCursor VERSION 1.0.0)

set(EXECUTABLE_OUTPUT_PATH "${CMAKE_CURRENT_SOURCE_DIR}/bin")
message("ouput set to ${EXECUTABLE_OUTPUT_PATH}")
add_subdirectory("src/Service")
add_subdirectory("src/GUI")