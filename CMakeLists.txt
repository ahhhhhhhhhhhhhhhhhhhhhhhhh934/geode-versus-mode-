cmake_minimum_required(VERSION 3.21)
project(VersusMode VERSION 1.0.5)

add_library(${PROJECT_NAME} SHARED
    src/main.cpp
    src/ui/LobbyLayer.cpp
    src/ui/RoomPages.cpp
    src/ui/MatchPages.cpp
)

# Geode's CLI provides GEODE_SDK when building with geode build.
if (NOT DEFINED ENV{GEODE_SDK})
    message(FATAL_ERROR "GEODE_SDK is not set. Install the Geode SDK and build with the Geode CLI.")
endif()

add_subdirectory($ENV{GEODE_SDK} ${CMAKE_CURRENT_BINARY_DIR}/geode)

# Geode 5.11.2's iOS codegen emits FMOD wrapper signatures in
# GeneratedSource.cpp without including FMOD's declarations. The iOS
# bindings target therefore needs the FMOD header forced into that TU.
if (GEODE_TARGET_PLATFORM STREQUAL "iOS" AND TARGET GeodeBindings)
    target_compile_options(GeodeBindings PRIVATE "-include" "fmod.hpp")
endif()

# Globed is not published in the Geode index yet, so declare it as an
# externally supplied runtime dependency. We still fetch its public headers
# from the upstream source repository for compilation.
#
# The exact version here is verified by the runtime check; it must satisfy the
# comparison the mod publishes in mod.json. Use the exact Globed version
# the mod was built against, not a comparator.
setup_geode_mod(${PROJECT_NAME} EXTERNALS dankmeme.globed2:2.3.0-beta.1)

include(FetchContent)
FetchContent_Declare(
    globed_headers
    GIT_REPOSITORY https://github.com/GlobedGD/globed2.git
    GIT_TAG 737d2211b6ce00119f752dc07d0d76d2700b6d08
    GIT_SHALLOW FALSE
)
FetchContent_GetProperties(globed_headers)
if (NOT globed_headers_POPULATED)
    FetchContent_Populate(globed_headers)
endif()

target_include_directories(${PROJECT_NAME} PRIVATE
    ${globed_headers_SOURCE_DIR}/include
)
