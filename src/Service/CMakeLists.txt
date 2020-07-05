cmake_minimum_required(VERSION 3.0.0)
set(CMAKE_CXX_STANDARD 14)
# prevent test targets
set_property(GLOBAL PROPERTY CTEST_TARGETS_ADDED 1)

project(CommunistCursor_Service VERSION 0.1.0)

set(SUBMODULE_DIR "submodules")

set(JSON_BuildTests OFF CACHE INTERNAL "")
add_subdirectory(${SUBMODULE_DIR}/json)

include(CTest)
enable_testing()

message("CMAKE_SYSTEM_NAME: ${CMAKE_SYSTEM_NAME}")

file(GLOB CC_MAIN "main.cpp")
file(GLOB CC_SOURCE "./CC/*.cpp" "./CC/*.h")
file(GLOB CC_DISPATCH  "./Dispatcher/*.cpp" "./Dispatcher/*.h")
file(GLOB CC_OSINTERFACE "OSInterface/*.cpp" "./OSInterface/*.h")
file(GLOB CC_SOCKET "./Socket/*.cpp" "./Socket/*.h")
file(GLOB SUB_ARGS "${SUBMODULE_DIR}/args/args.hxx")
file(GLOB SUB_JSON "${SUBMODULE_DIR}/json/include/nlohmann/*.hpp" "${SUBMODULE_DIR}/json/include/nlohmann/*.cpp" "${SUBMODULE_DIR}/json/include/nlohmann/*.h")

source_group(TREE "src/service" FILES ${CC_SOURCE})
source_group(TREE "src/service" FILES ${CC_OSINTERFACE})
source_group(TREE "src/service" FILES ${CC_SOCKET})
source_group(TREE "src/service" FILES ${CC_DISPATCH})

source_group("Submodules\\args" FILES ${SUB_ARGS})
source_group("Submodules\\json" FILES ${SUB_JSON})
source_group("Main" FILES ${CC_MAIN})

if (${CMAKE_SYSTEM_NAME} MATCHES "Windows")
        file(GLOB CC_WINDOWS "./Windows/*.cpp" "./Windows/*.h")
		source_group("Windows" FILES ${CC_WINDOWS})
        add_executable(CommunistCursor ${CC_MAIN} ${CC_SOURCE} ${CC_DISPATCH} ${CC_OSINTERFACE} ${CC_SOCKET} ${SUB_ARGS} ${CC_WINDOWS} ${SUB_JSON} )
        target_link_libraries(CommunistCursor ws2_32.lib IPHLPAPI nlohmann_json::nlohmann_json Wtsapi32.dll)
endif()

if(${CMAKE_SYSTEM_NAME} MATCHES "Linux")
        file(GLOB CC_LINUX "./Linux/*.cpp" "./Linux/*.h")
		source_group("Linux" FILES ${CC_LINUX})
        add_executable(CommunistCursor ${CC_MAIN} ${CC_SOURCE} ${CC_DISPATCH} ${CC_OSINTERFACE} ${CC_SOCKET} ${SUB_ARGS} ${CC_LINUX} ${SUB_JSON} )
endif()

if(${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
        file(GLOB CC_APPLE "./MacOS/*.cpp" "./MacOS/*.h")
        file(GLOB CC_APPLE_MM "./MacOS/*.mm")
		source_group("Apple" FILES ${CC_APPLE} ${CC_APPLE_MM})
        set_source_files_properties(${CC_APPLE_MM} PROPERTIES COMPILE_FLAGS "-x objective-c++")
        add_executable(CommunistCursor ${CC_MAIN} ${CC_SOURCE} ${CC_DISPATCH} ${CC_OSINTERFACE} ${CC_SOCKET} ${SUB_ARGS} ${CC_APPLE} ${CC_APPLE_MM} ${SUB_JSON} )
        #set_target_properties(CommunistCursor PROPERTIES MACOSX_BUNDLE TRUE MACOSX_BUNDLE_INFO_PLIST "Info.plist")
        
        find_library(IOKIT IOKit)
        find_library(FOUNDATION Foundation)
        find_library(APPLICATION_SERVICES ApplicationServices)
        
        target_link_libraries(CommunistCursor ${IOKIT} ${FOUNDATION} ${APPLICATION_SERVICES} nlohmann_json::nlohmann_json)
endif()

if(${CMAKE_SYSTEM_NAME} MATCHES "Unix")
        file(GLOB CC_UNIX "./Unix/*.cpp")
		source_group("Unix" FILES ${CC_UNIX})
        add_executable(CommunistCursor ${CC_MAIN} ${CC_SOURCE} ${CC_DISPATCH} ${CC_OSINTERFACE} ${CC_SOCKET} ${SUB_ARGS} ${CC_UNIX} ${SUB_JSON} )
endif()

target_include_directories(CommunistCursor PRIVATE "${SUBMODULE_DIR}/args")
target_include_directories(CommunistCursor PRIVATE "${SUBMODULE_DIR}/json/include")

set(CPACK_PROJECT_NAME ${PROJECT_NAME})
set(CPACK_PROJECT_VERSION ${PROJECT_VERSION})
include(CPack)

install(TARGETS CommunistCursor RUNTIME DESTINATION bin)