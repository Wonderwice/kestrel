# Kestrel

[![CI/CD Pipeline](https://github.com/Wonderwice/kestrel/actions/workflows/ci.yml/badge.svg)](https://github.com/Wonderwice/kestrel/actions)
[![Documentation](https://img.shields.io/badge/docs-online-blue.svg)](https://Wonderwice.github.io/kestrel/)

A rendering engine built in C++/CUDA.

## Current Features (v0.1)

- ✅ Basic ray-sphere intersection
- ✅ Simple pinhole camera model
- ✅ Surface normal visualization
- ✅ PPM image output
- ✅ Full Doxygen documentation
- ✅ Automated CI/CD pipeline
- ✅ GitHub Pages deployment

## Project Structure

```
.
├── .github/
│   └── workflows/
│       └── ci.yml           # CI/CD pipeline
├── CMakeLists.txt           # Build configuration
├── Doxyfile                 # Doxygen documentation config
├── .clang-format            # Code style rules
├── .gitignore               # Git ignore patterns
├── main.cpp                 # Main renderer loop
├── vec3.h                   # 3D vector math (GPU-ready)
├── ray.h                    # Ray representation
├── sphere.h                 # Sphere primitive with intersection
└── camera.h                 # Camera and ray generation
```

## Quick Start

```bash
git clone https://github.com/Wonderwice/kestrel.git
cd kestrel
mkdir build && cd build
cmake ..
make
./kestrel
```

This will generate `output.ppm` in the build directory.

## Documentation

**Online:** [https://Wonderwice.github.io/kestrel/](https://Wonderwice.github.io/kestrel/)

**Local generation:**
```bash
doxygen Doxyfile
firefox docs/html/index.html
```

The documentation includes:
- Detailed class and function descriptions
- Call graphs and dependency diagrams
- Source code cross-references
- Mathematical explanations of algorithms

## CI/CD Pipeline

Every push to `main` automatically:
1. ✅ Builds on Ubuntu and macOS (Release + Debug)
2. ✅ Runs test renders to verify correctness
3. ✅ Generates and deploys documentation to GitHub Pages
4. ✅ Checks code formatting and quality
5. ✅ Uploads render artifacts

View pipeline results: [GitHub Actions](https://github.com/Wonderwice/kestrel/actions)

## Viewing Output

### On Linux/Mac:
```bash
# Convert to PNG
convert output.ppm output.png

# Or view directly
display output.ppm  # ImageMagick
feh output.ppm      # feh
```

### On Windows:
- Use IrfanView, GIMP, or Photoshop
- Or convert online at https://convertio.co/ppm-png/

### Using Python:
```python
from PIL import Image
img = Image.open('output.ppm')
img.show()
```

## Architecture Notes

All geometric primitives and math operations are decorated with `__host__ __device__` for future CUDA compatibility. The codebase is structured to allow easy porting to GPU.

## Resources

- [Ray Tracing in One Weekend](https://raytracing.github.io/)
- [PBRT Book](https://www.pbr-book.org/)
- [Scratchapixel](https://www.scratchapixel.com/)