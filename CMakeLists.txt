cmake_minimum_required(VERSION 3.18)
project(Kestrel LANGUAGES CXX)

# C++ Standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Optimization flags
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

set(CMAKE_CXX_FLAGS_RELEASE "-O3")
set(CMAKE_CXX_FLAGS_DEBUG "-g -O0")

# Emit the executable at the build-tree root (next to the copied data/), even
# though its target is defined under src/.
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})

# Strict warnings, shared by every project target.
set(KESTREL_WARNINGS -Wall -Wextra -Wpedantic -Wreorder -Wshadow -Wnon-virtual-dtor)

# ---------------------------------------------------------------------------
# kestrel_core : all renderer logic, as a static library linked by the CLI
# executable (and future tests). Sources are contributed by the per-directory
# CMakeLists under src/ via target_sources().
#
# Include model:
#   PUBLIC  include/      - the library's public API (what the exe may use).
#   PRIVATE src/, src/*/  - implementation-only headers (materials, shapes,
#                           math, constants); never visible to the exe.
#   PRIVATE third_party/  - vendored single-header libs (impl only).
# ---------------------------------------------------------------------------
add_library(kestrel_core STATIC)

target_include_directories(kestrel_core
    PUBLIC  ${CMAKE_CURRENT_SOURCE_DIR}/include
    PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src
            ${CMAKE_CURRENT_SOURCE_DIR}/third_party)

target_link_libraries(kestrel_core PUBLIC z)
target_compile_options(kestrel_core PRIVATE ${KESTREL_WARNINGS})

# Pull in the library sources and the CLI executable (src/CMakeLists.txt).
add_subdirectory(src)

# Unit tests (doctest). On by default; turn off with -DKESTREL_BUILD_TESTS=OFF
# to build only the CLI.
option(KESTREL_BUILD_TESTS "Build the kestrel_core unit tests" ON)
if(KESTREL_BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()

# Copy scene/asset data next to the build output.
file(COPY ${CMAKE_SOURCE_DIR}/data/ DESTINATION ${CMAKE_BINARY_DIR}/data)

# Link-time / interprocedural optimization (Release only). Lets the compiler
# inline hot cross-TU calls (Vec3/Ray math, BSDFs) at link time.
include(CheckIPOSupported)
check_ipo_supported(RESULT ipo_ok OUTPUT ipo_msg)
if(ipo_ok)
    set_property(TARGET kestrel_core PROPERTY INTERPROCEDURAL_OPTIMIZATION_RELEASE ON)
    set_property(TARGET kestrel      PROPERTY INTERPROCEDURAL_OPTIMIZATION_RELEASE ON)
    message(STATUS "IPO/LTO enabled for Release builds")
else()
    message(STATUS "IPO/LTO not supported: ${ipo_msg}")
endif()

message(STATUS "Build type: ${CMAKE_BUILD_TYPE}")
message(STATUS "C++ compiler: ${CMAKE_CXX_COMPILER}")
message(STATUS "C++ flags: ${CMAKE_CXX_FLAGS_RELEASE}")
