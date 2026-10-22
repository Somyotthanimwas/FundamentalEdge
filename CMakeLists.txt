cmake_minimum_required(VERSION 3.20)
project(FundamentalUpdater_rev12 LANGUAGES CXX)

if(NOT WIN32)
    message(FATAL_ERROR "FundamentalUpdater Rev12 is a Windows application and must be built with a Windows toolchain.")
endif()

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_executable(FundamentalUpdater_rev12
    source/main.cpp
    source/SwingSignal.cpp
)

target_include_directories(FundamentalUpdater_rev12 PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/source")

if(MINGW)
    target_link_options(FundamentalUpdater_rev12 PRIVATE -static -static-libgcc -static-libstdc++)
endif()

set_target_properties(FundamentalUpdater_rev12 PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/bin"
)
