#include "common.h"
#include "rng.h"
#include "peripherals/rng.h"

u64 rand(u64 min, u64 max) {
  if (!((*RNG_CTRL) & 1)) { // Is init needed?
    *RNG_STATUS = 0x40000;
    *RNG_INT_MASK |= 1;
    *RNG_CTRL |= 1; // Enable
    while (!((*RNG_STATUS) >> 24)); // Wait until entropy good enough
  }
  return ((((u64)(*RNG_DATA) << 32) | *RNG_DATA) % (max - min)) + min;
}
