#ifndef PCG32_H
#define PCG32_H

#include <cstdint>

// PCG32 Random Number Generator
// Based on the PCG family by Melissa O'Neill
class PCG32 {
public:
  PCG32(uint64_t seed = 0x853c49e6748fea9bULL,
        uint64_t stream = 0xa02bdbf7bb3c0a7ULL)
      : state(0), inc((stream << 1u) | 1u) {
    next();
    state += seed;
    next();
  }

  uint32_t next() {
    uint64_t oldstate = state;
    state = oldstate * 6364136223846793005ULL + inc;
    uint32_t xorshifted =
        static_cast<uint32_t>(((oldstate >> 18u) ^ oldstate) >> 27u);
    uint32_t rot = static_cast<uint32_t>(oldstate >> 59u);
    return (xorshifted >> rot) | (xorshifted << ((-rot) & 31));
  }

  // Generate a random float in [0, 1)
  float next_float() {
    return static_cast<float>(next()) / static_cast<float>(0x100000000ULL);
  }

private:
  uint64_t state;
  uint64_t inc;
};

#endif // PCG32_Hs