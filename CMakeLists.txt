cmake_minimum_required(VERSION 3.21)
project(VersusMode VERSION 0.6.7)

add_library(${PROJECT_NAME} SHARED
    src/main.cpp
)

# Geode's CLI provides GEODE_SDK when building with geode build.
if (NOT DEFINED ENV{GEODE_SDK})
    message(FATAL_ERROR "GEODE_SDK is not set. Install the Geode SDK and build with the Geode CLI.")
endif()

add_subdirectory($ENV{GEODE_SDK} ${CMAKE_CURRENT_BINARY_DIR}/geode)
# Globed is distributed as a mod, not a Geode SDK-index package.
# Fetch its public headers at the pinned stable v2.2.2 tag.
include(FetchContent)
FetchContent_Declare(
    globed_headers
    GIT_REPOSITORY https://github.com/GlobedGD/globed2.git
    GIT_TAG v2.2.2
    GIT_SHALLOW FALSE
)
FetchContent_GetProperties(globed_headers)
if (NOT globed_headers_POPULATED)
    FetchContent_Populate(globed_headers)
endif()

target_include_directories(${PROJECT_NAME} PRIVATE ${globed_headers_SOURCE_DIR}/include)

setup_geode_mod(${PROJECT_NAME} EXTERNALS dankmeme.globed2:2.2.2)
