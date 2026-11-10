cmake_minimum_required(VERSION 3.21)
project(VersusMode VERSION 0.6.15)

add_library(${PROJECT_NAME} SHARED
    src/main.cpp
)

# Geode's CLI provides GEODE_SDK when building with geode build.
if (NOT DEFINED ENV{GEODE_SDK})
    message(FATAL_ERROR "GEODE_SDK is not set. Install the Geode SDK and build with the Geode CLI.")
endif()

add_subdirectory($ENV{GEODE_SDK} ${CMAKE_CURRENT_BINARY_DIR}/geode)

# Pull the Globed beta dependency through Geode's dependency resolver so the
# headers and runtime mod package always come from the same pinned release.
setup_geode_mod(${PROJECT_NAME} EXTERNALS dankmeme.globed2:2.3.0)

# Globed's Soft Link API is header-only. The dependency resolver downloads its
# headers into geode-deps; do not fetch an unrelated moving nightly branch.
target_include_directories(${PROJECT_NAME} PRIVATE
    ${CMAKE_CURRENT_BINARY_DIR}/geode-deps/dankmeme.globed2/include
)
