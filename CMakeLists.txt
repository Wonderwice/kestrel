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
    src/bsdfs/conductor.cpp
    src/bsdfs/lambertian.cpp
    src/vec3.cpp
    src/camera.cpp
    src/light.cpp
    src/ray.cpp
    src/scene.cpp
    src/sphere.cpp
    src/triangle.cpp
    src/plymesh.cpp
)

# Headers
set(HEADERS
    include/vec3.h
    include/ray.h
    include/sphere.h
    include/camera.h
    include/light.h
    include/scene.h
    include/pcg32.h
    include/kestrel.h
    include/triangle.h
    include/plymesh.h
)

file(COPY ${CMAKE_SOURCE_DIR}/data/ DESTINATION ${CMAKE_BINARY_DIR}/data)

# Executable
add_executable(kestrel ${SOURCES} ${HEADERS})

# Include directories
target_include_directories(kestrel PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include)

# Print build info
message(STATUS "Build type: ${CMAKE_BUILD_TYPE}")
message(STATUS "C++ compiler: ${CMAKE_CXX_COMPILER}")
message(STATUS "C++ flags: ${CMAKE_CXX_FLAGS_RELEASE}")