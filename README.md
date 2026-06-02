# Kestrel

A physically-based CPU path tracer written in C++. Kestrel reads scenes in a
subset of the [Mitsuba 0.6](https://www.mitsuba-renderer.org) XML format and
renders them with unidirectional path tracing, Multiple Importance Sampling
(MIS), and Next Event Estimation (NEE).

![Sponza with a glass sphere, rendered by Kestrel](docs/images/sponza_master.png)

*The Sponza atrium with a dielectric sphere under a large area light, rendered
with the path integrator.*

## Quick Start

```bash
git clone https://github.com/Wonderwice/kestrel.git
cd kestrel
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j$(nproc)

./build/kestrel data/scenes/cbox/cbox.xml -o render.exr
```

The binary is placed at `build/kestrel`. Image resolution and the default
samples per pixel are read from the scene file; the runtime flags are:

```
kestrel <scene.xml> [-o <output>] [-i path|direct|direct_bsdf] [-s <spp>] [-t <threads>]
```

| Flag | Default | Description |
|------|---------|-------------|
| `<scene.xml>` | *(required)* | Scene file in Mitsuba XML format. |
| `-o <output>` | `output.exr` | Output path. The extension selects the format: `.exr` (linear HDR), `.png` (sRGB 8-bit), `.ppm` (sRGB 8-bit fallback). |
| `-i <integrator>` | `path` | `path` = full global illumination with MIS; `direct` = single-bounce light sampling (NEE); `direct_bsdf` = single-bounce BSDF sampling. |
| `-s, --spp <n>` | *(scene)* | Override the scene's `sampleCount`. |
| `-t, --threads <n>` | all cores | Number of render threads. |

Two example scenes ship with the repository so you can render immediately after
cloning: the traditional Cornell box (`data/scenes/cbox/cbox.xml`) and the
Veach multiple-importance-sampling test (`data/scenes/veach_mi/mi.xml`).

### Integrator comparison

Kestrel's `direct` and `direct_bsdf` modes isolate the two single-strategy
estimators that MIS combines. Rendering the Veach scene three ways shows why
MIS wins — see [the Integrators guide](http://wonderwice.com/kestrel/integrators/):

```bash
./build/kestrel data/scenes/veach_mi/mi.xml -i direct_bsdf -s 32 -o veach_bsdf.exr  # BSDF sampling only
./build/kestrel data/scenes/veach_mi/mi.xml -i direct      -s 32 -o veach_light.exr # light sampling only
./build/kestrel data/scenes/veach_mi/mi.xml -i path        -s 32 -o veach_path.exr  # full path tracer w/ MIS
```

## Documentation

The authoring guide (scene format, sensors, emitters, BSDFs, shapes,
integrators) lives in [`docs/`](docs/) and is published online:

**[http://wonderwice.com/kestrel](http://wonderwice.com/kestrel)**

To preview the docs locally (MkDocs Material):

```bash
pip install mkdocs-material
mkdocs serve   # then open http://127.0.0.1:8000
```

API-level documentation can additionally be generated from the source comments
with `doxygen Doxyfile`.

## Viewing output

`.exr` is the recommended output: linear, 32-bit float, ideal for tone mapping
and compositing. Inspect or convert it with OpenImageIO:

```bash
oiiotool render.exr --colorconvert linear sRGB -o render.png   # to displayable PNG
oiiotool --stats render.exr                                    # min/max/avg, NaN/Inf counts
```

Render straight to a displayable image by choosing the extension:

```bash
./build/kestrel data/scenes/cbox/cbox.xml -o render.png
```

## Tests

```bash
./build/kestrel_tests
```

## Acknowledgements

Kestrel was inspired from the **Torrey** renderer framework used in
**UCSD CSE 168** (Computer Graphics II: Rendering), created by
[Tzu-Mao Li](https://cseweb.ucsd.edu/~tzli/). The course scaffolding and
assignment scenes shaped the early structure of this project.

Example scenes and assets:

- **Cornell box** — the classic global-illumination test by the Cornell
  Program of Computer Graphics.
- **Veach MIS scene** (`data/scenes/veach_mi`) — from Eric Veach's thesis,
  modeled after a file by Steve Marschner (Cornell CS667).
- **Sponza** — the Atrium Sponza model by Marko Dabrović, refreshed by
  Frank Meinl / Crytek.

## Resources

- [Ray Tracing in One Weekend](https://raytracing.github.io/)
- [PBR Book](https://www.pbr-book.org/)
- [Scratchapixel](https://www.scratchapixel.com/)
