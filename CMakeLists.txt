cmake_minimum_required(VERSION 3.0.0)
set(CMAKE_CXX_STANDARD 14)

project(CommunistCursor VERSION 0.1.0)

include(CTest)
enable_testing()

file(GLOB CommunistCurosr_SOURCE "src/*.cpp")

add_executable(CommunistCursor ${CommunistCurosr_SOURCE})

if (${CMAKE_SYSTEM_NAME} MATCHES "Windows")
        target_link_libraries(CommunistCursor ws2_32.lib)
endif()

set(CPACK_PROJECT_NAME ${PROJECT_NAME})
set(CPACK_PROJECT_VERSION ${PROJECT_VERSION})
include(CPack)
