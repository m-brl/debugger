cmake_minimum_required(VERSION 3.27.4)
project(debugger)

find_package(PkgConfig REQUIRED)
pkg_check_modules(LIBDWARF REQUIRED libdwarf)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(SOURCE_FILE
    src/main.cpp
    src/utils.cpp
    src/File.cpp
    src/Logger.cpp
    src/AddressMap.cpp
)

add_executable(${PROJECT_NAME} ${SOURCE_FILE})

target_compile_options(${PROJECT_NAME} PRIVATE -Wall -Wextra)

if (CMAKE_BUILD_TYPE STREQUAL "Debug")
    target_compile_definitions(${PROJECT_NAME} PRIVATE DEBUG_MODE)
elseif (CMAKE_BUILD_TYPE STREQUAL "Release")
    target_compile_options(${PROJECT_NAME} PRIVATE -O2)
endif()

target_link_libraries(${PROJECT_NAME} ${LIBDWARF_LIBRARIES})
target_include_directories(${PROJECT_NAME} PRIVATE
    include
    ${LIBDWARF_INCLUDE_DIRS}
)
