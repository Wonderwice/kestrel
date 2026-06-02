# ---------------------------------------------------------------------------
# kestrel_tests : doctest unit tests for kestrel_core.
#
# Unlike the CLI (which sees only kestrel_core's PUBLIC headers), the tests
# also exercise implementation-only classes (BSDFs, concrete shapes, Mat4,
# constants), so they are given src/ and third_party/ on the include path in
# addition to linking the static library.
# ---------------------------------------------------------------------------
add_executable(kestrel_tests
    test_main.cpp
    test_vec3.cpp
    test_ray.cpp
    test_aabb.cpp
    test_pcg32.cpp
    test_mat4.cpp
    test_camera.cpp
    test_light.cpp
    test_texture.cpp
    test_bsdfs.cpp
    test_shapes.cpp
    test_bvh.cpp
    test_scene.cpp
    test_integrators.cpp
    test_scene_params.cpp
    test_logger.cpp)

target_include_directories(kestrel_tests PRIVATE
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_SOURCE_DIR}/third_party)

target_link_libraries(kestrel_tests PRIVATE kestrel_core)
target_compile_options(kestrel_tests PRIVATE ${KESTREL_WARNINGS})

add_test(NAME kestrel_tests COMMAND kestrel_tests)
