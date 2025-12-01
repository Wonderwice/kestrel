# Kestrel

[![CI/CD Pipeline](https://github.com/Wonderwice/kestrel/actions/workflows/ci.yml/badge.svg)](https://github.com/Wonderwice/kestrel/actions)
[![Documentation](https://img.shields.io/badge/docs-online-blue.svg)](https://Wonderwice.github.io/kestrel/)

A rendering engine built in C++/CUDA.

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