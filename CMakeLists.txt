cmake_minimum_required(VERSION 3.0.0)
set(CMAKE_CXX_STANDARD 14)

project(CommunistCursor VERSION 0.1.0)

include(CTest)
enable_testing()

file(GLOB CommunistCurosr_SOURCE "src/*.cpp")
file(GLOB CommunistCurosr_Windows_SOURCE "src/Windows/*.cpp")
file(GLOB CommunistCurosr_Linux_SOURCE "src/Linux/*.cpp")
file(GLOB CommunistCurosr_Apple_SOURCE "src/Apple/*.cpp")

if (${CMAKE_SYSTEM_NAME} MATCHES "Windows")
        add_executable(CommunistCursor ${CommunistCurosr_SOURCE} ${CommunistCurosr_Windows_SOURCE} )
        target_link_libraries(CommunistCursor ws2_32.lib)
endif()

if(${CMAKE_SYSTEM_NAME} MATCHES "Linux")
        add_executable(CommunistCursor ${CommunistCurosr_SOURCE} ${CommunistCurosr_Linux_SOURCE} )
endif()

if(${CMAKE_SYSTEM_NAME} MATCHES "Apple")
        add_executable(CommunistCursor ${CommunistCurosr_SOURCE} ${CommunistCurosr_Apple_SOURCE} )
endif()

set(CPACK_PROJECT_NAME ${PROJECT_NAME})
set(CPACK_PROJECT_VERSION ${PROJECT_VERSION})
include(CPack)
