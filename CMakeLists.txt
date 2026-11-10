cmake_minimum_required(VERSION 3.21)
project(VersusMode VERSION 0.6.18)

add_library(${PROJECT_NAME} SHARED
    src/main.cpp
)

# Geode's CLI provides GEODE_SDK when building with geode build.
if (NOT DEFINED ENV{GEODE_SDK})
    message(FATAL_ERROR "GEODE_SDK is not set. Install the Geode SDK and build with the Geode CLI.")
endif()

add_subdirectory($ENV{GEODE_SDK} ${CMAKE_CURRENT_BINARY_DIR}/geode)

# Let Geode resolve the dependency declared in mod.json and download its
# package headers into geode-deps. EXTERNALS would only mark it as externally
# provided, which leaves the include directory empty in CI.
setup_geode_mod(${PROJECT_NAME})

# Globed's Soft Link API is header-only. The dependency resolver downloads its
# headers into geode-deps; do not fetch an unrelated moving nightly branch.
target_include_directories(${PROJECT_NAME} PRIVATE
    ${CMAKE_CURRENT_BINARY_DIR}/geode-deps/dankmeme.globed2/include
)
