cmake_minimum_required(VERSION 3.21)
project(VersusMode VERSION 0.3.0)

add_library(${PROJECT_NAME} SHARED
    src/main.cpp
)

# Geode's CLI provides GEODE_SDK when building with geode build.
if (NOT DEFINED ENV{GEODE_SDK})
    message(FATAL_ERROR "GEODE_SDK is not set. Install the Geode SDK and build with the Geode CLI.")
endif()

add_subdirectory($ENV{GEODE_SDK} ${CMAKE_CURRENT_BINARY_DIR}/geode)
setup_geode_mod(${PROJECT_NAME} EXTERNALS dankmeme.globed2:>=2.3.0-beta.1)

include(FetchContent)
FetchContent_Declare(globed2_headers
    GIT_REPOSITORY https://github.com/GlobedGD/globed2.git
    GIT_TAG main
    GIT_SHALLOW TRUE
)
FetchContent_GetProperties(globed2_headers)
if (NOT globed2_headers_POPULATED)
    FetchContent_Populate(globed2_headers)
endif()
target_include_directories(${PROJECT_NAME} PRIVATE
    ${globed2_headers_SOURCE_DIR}/include
)

