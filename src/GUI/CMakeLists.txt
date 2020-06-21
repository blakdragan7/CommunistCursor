cmake_minimum_required(VERSION 3.0.0)
# prevent test targets
set_property(GLOBAL PROPERTY CTEST_TARGETS_ADDED 1)

project(CommunistCursor_GUI VERSION 0.1.0)

include(CTest)
enable_testing()

find_package(Python3 COMPONENTS Interpreter Development)
if (NOT ${Python3_FOUND})
    message(FATAL_ERROR "Could not locate Python. Python 3.x Required")
endif()
message("python found: ${Python3_EXECUTABLE}")
message("output bin")
file(GLOB CCG_PYTHON_SOURCE "*.py")
source_group("src" FILES ${CCG_PYTHON_SOURCE})

if (${CMAKE_SYSTEM_NAME} MATCHES "Windows")
    set(CCGUI_EXECUTABLE "CCGui.exe")
elseif (${CMAKE_SYSTEM_NAME} MATCHES "Apple")
    set(CCGUI_EXECUTABLE "CCGui.app")
else()
    set(CCGUI_EXECUTABLE "CCGui")
endif()

add_custom_command(OUTPUT ${CCGUI_EXECUTABLE} COMMAND ${Python3_EXECUTABLE} ARGS "${CMAKE_CURRENT_SOURCE_DIR}/GenerateExe.py" "${EXECUTABLE_OUTPUT_PATH}")
add_custom_target(CCG_GUI SOURCES ${CCG_PYTHON_SOURCE} DEPENDS ${CCGUI_EXECUTABLE})

set(CPACK_PROJECT_NAME ${PROJECT_NAME})
set(CPACK_PROJECT_VERSION ${PROJECT_VERSION})
include(CPack)