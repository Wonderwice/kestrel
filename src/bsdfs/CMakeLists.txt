# Material / BSDF implementations. Private headers (the concrete materials and
# the onb.h helper) live here next to their sources.
target_sources(kestrel_core PRIVATE
    conductor.cpp
    dielectric.cpp
    lambertian.cpp
    microfacet.cpp
    phong.cpp
    blinn.cpp
    blinn_microfacet.cpp
    plastic.cpp
    rough_dielectric.cpp)
target_include_directories(kestrel_core PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
