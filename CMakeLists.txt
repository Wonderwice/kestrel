cmake_minimum_required(VERSION 3.18)
project(Kestrel LANGUAGES CXX)

# C++ Standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Optimization flags
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

set(CMAKE_CXX_FLAGS_RELEASE "-O3 -march=native")
set(CMAKE_CXX_FLAGS_DEBUG "-g -O0")

# Source files
set(SOURCES
    src/kestrel.cpp
)

# Headers (for IDE)
set(HEADERS
    src/vec3.h
    src/ray.h
    src/sphere.h
    src/camera.h
)

# Executable
add_executable(kestrel ${SOURCES} ${HEADERS})

# Include directories
target_include_directories(kestrel PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)

# Print build info
message(STATUS "Build type: ${CMAKE_BUILD_TYPE}")
message(STATUS "C++ compiler: ${CMAKE_CXX_COMPILER}")
message(STATUS "C++ flags: ${CMAKE_CXX_FLAGS_RELEASE}")