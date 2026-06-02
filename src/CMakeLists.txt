# Top-level library sources (core types, scene, parser, asset loading).
target_sources(kestrel_core PRIVATE
    vec3.cpp
    ray.cpp
    camera.cpp
    light.cpp
    scene.cpp
    logger.cpp
    texture.cpp
    envmap.cpp
    parser.cpp
    ${CMAKE_SOURCE_DIR}/third_party/tinyxml2.cpp)

# Per-module sources + their private header directories.
add_subdirectory(math)
add_subdirectory(accel)
add_subdirectory(bsdfs)
add_subdirectory(shapes)
add_subdirectory(integrators)
add_subdirectory(io)

# ---------------------------------------------------------------------------
# kestrel : thin CLI front-end (arg parsing, render loop, image output). Sees
# only kestrel_core's PUBLIC headers, which enforces the public/private split.
# ---------------------------------------------------------------------------
add_executable(kestrel kestrel.cpp)
target_link_libraries(kestrel PRIVATE kestrel_core)
target_compile_options(kestrel PRIVATE ${KESTREL_WARNINGS})
