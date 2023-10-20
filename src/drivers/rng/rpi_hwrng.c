#include <common.h>
#include <rpi_hwrng.h>

#if RPI_VERSION == 3

#include <bcm2835_rng.h>

u64 rpi_hwrng_rand(u64 min, u64 max) {
  return bcm2835_rand(min, max);
}

#elif RPI_VERSION == 4

#include <iproc_rng200.h>

u64 rpi_hwrng_rand(u64 min, u64 max) {
  return iproc_rng200_rand(min, max);
}

#endif
