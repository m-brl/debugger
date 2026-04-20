cmake_minimum_required(VERSION 3.27.4)
project(debugger)

add_compile_options(-Wall -Wextra)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

include_directories(include)

set(SOURCE_FILE
    src/main.cpp
    src/File.cpp
    src/Logger.cpp
    src/Tracee.cpp
)

add_executable(debugger ${SOURCE_FILE})
