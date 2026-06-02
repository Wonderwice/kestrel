# Architecture

This page describes how Kestrel turns a scene file into an image. It complements
the authoring guide (scene format, BSDFs, shapes, …) by explaining the internals.

## Pipeline overview

```
scene.xml ──▶ parser ──▶ Scene ──▶ BVH build ──▶ Integrator.render ──▶ image_io ──▶ output.{exr,png,ppm}
```

1. **Parse.** `read_from_file` (`src/parser.cpp`) reads a subset of the Mitsuba
   0.6 XML format with tinyxml2, instantiating shapes, materials, emitters, the
   sensor/camera, and the integrator settings into a `Scene`.
2. **Build.** `Scene::build` (`src/scene.cpp`) constructs the BVH over all
   primitives and collects the emitter list used by next-event estimation.
3. **Render.** The CLI (`src/kestrel.cpp`) selects an integrator and calls
   `Integrator::render`, which splits the image into scanlines handed out
   atomically to worker threads and drives `integrate()` once per sample.
4. **Write.** `write_image` (`src/io/image_io.cpp`) encodes the linear `Color`
   buffer; the output extension selects EXR (linear float), PNG, or PPM (sRGB).

## Core abstractions

- **`Scene`** (`include/scene.h`) — owns geometry, materials, emitters, and the
  BVH. Exposes `hit()` / `occluded()` for traversal and `emissive_pdf()` for MIS.
- **`Integrator`** (`include/integrator.h`) — base class providing the threaded
  `render()` loop and the `mis_power()` power heuristic (β=2); subclasses
  implement `integrate(ray, scene, rng) -> Color`. Implementations:
  `PathIntegrator` (full GI with MIS) and `DirectIntegrator` (single bounce, in
  light-sampling or BSDF-sampling mode).
- **`Material`** (`include/material.h`) — the BSDF interface. The key methods are
  `scatter()` (importance-samples a continuation ray, returns its pdf — 0 signals
  a delta/specular lobe), `eval()` (directional BRDF for NEE), `pdf()` (for MIS
  weighting), `emitted()`, and `is_specular()`.
- **Shapes** (`src/shapes/`) — `Sphere`, `Rectangle`, `Triangle`, and the mesh
  loaders (OBJ, PLY, Mitsuba serialized). Each provides intersection and, for
  emitters, area-light sampling.

## Light transport

`PathIntegrator::integrate` (`src/integrators/path.cpp`) is the heart of the
renderer. Each non-specular bounce combines two estimators with multiple
importance sampling (Veach's multi-sample model, power heuristic):

- **Next-event estimation (NEE):** shadow rays to sampled points on area
  emitters, point lights, and the environment map.
- **BSDF sampling:** the scattered continuation ray; emission it hits on the next
  iteration is MIS-weighted against the NEE pdf.

Specular/delta lobes skip NEE and carry weight 1. Paths terminate by Russian
roulette after three bounces.

## Acceleration

The BVH (`include/accel/bvh.h`, `src/accel/bvh.cpp`) is a flattened bounding
volume hierarchy over an AABB tree (`include/accel/aabb.h`), traversed iteratively
for both closest-hit (`hit`) and any-hit shadow (`occluded`) queries.

## Numerics & determinism

`pcg32` (`include/pcg32.h`) provides per-thread random streams. Shared epsilons
and the default far clip live in `src/constants.h`. Builds use `-O3` with LTO;
strict IEEE semantics are kept (no `-ffast-math`) so NaN/Inf handling and results
stay reproducible.
