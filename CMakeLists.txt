cmake_minimum_required(VERSION 3.0.0)
set(CMAKE_CXX_STANDARD 14)
# prevent test targets
set_property(GLOBAL PROPERTY CTEST_TARGETS_ADDED 1)

project(CommunistCursor VERSION 0.1.0)

include(CTest)
enable_testing()

message("CMAKE_SYSTEM_NAME: ${CMAKE_SYSTEM_NAME}")

file(GLOB CommunistCurosr_SOURCE "src/*.cpp" "src/OSInterface/*.cpp" "src/Socket/*.cpp" "src/OSInterface/*.h" "src/Socket/*.h")

if (${CMAKE_SYSTEM_NAME} MATCHES "Windows")
        file(GLOB CommunistCurosr_Windows_SOURCE "src/Windows/*.cpp")
        add_executable(CommunistCursor ${CommunistCurosr_SOURCE} ${CommunistCurosr_Windows_SOURCE} )
        target_link_libraries(CommunistCursor ws2_32.lib)
endif()

if(${CMAKE_SYSTEM_NAME} MATCHES "Linux")
        file(GLOB CommunistCurosr_Linux_SOURCE "src/Linux/*.cpp")
        add_executable(CommunistCursor ${CommunistCurosr_SOURCE} ${CommunistCurosr_Linux_SOURCE} )
endif()

if(${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
        file(GLOB CommunistCurosr_Apple_SOURCE "src/MacOS/*.cpp")
        add_executable(CommunistCursor ${CommunistCurosr_SOURCE} ${CommunistCurosr_Apple_SOURCE} )
endif()

if(${CMAKE_SYSTEM_NAME} MATCHES "Unix")
        file(GLOB CommunistCurosr_Unix_SOURCE "src/Unix/*.cpp")
        add_executable(CommunistCursor ${CommunistCurosr_SOURCE} ${CommunistCurosr_Unix_SOURCE} )
endif()

set(CPACK_PROJECT_NAME ${PROJECT_NAME})
set(CPACK_PROJECT_VERSION ${PROJECT_VERSION})
include(CPack)
