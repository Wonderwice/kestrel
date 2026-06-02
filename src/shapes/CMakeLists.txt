# Geometry implementations + their private headers (sphere, triangle,
# rectangle, mesh and the OBJ/PLY/serialized mesh loaders).
target_sources(kestrel_core PRIVATE
    sphere.cpp
    triangle.cpp
    rectangle.cpp
    mesh.cpp
    objmesh.cpp
    plymesh.cpp
    serializedmesh.cpp)
target_include_directories(kestrel_core PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
