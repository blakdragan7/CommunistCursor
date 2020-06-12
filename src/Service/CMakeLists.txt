cmake_minimum_required(VERSION 3.0.0)
set(CMAKE_CXX_STANDARD 14)
# prevent test targets
set_property(GLOBAL PROPERTY CTEST_TARGETS_ADDED 1)

project(CommunistCursor_GUI VERSION 0.1.0)

include(CTest)
enable_testing()



set(CPACK_PROJECT_NAME ${PROJECT_NAME})
set(CPACK_PROJECT_VERSION ${PROJECT_VERSION})
include(CPack)

install(TARGETS CommunistCursor RUNTIME DESTINATION bin)