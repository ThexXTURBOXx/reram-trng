#include <common.h>
#include <bcm2835_rng.h>
#include <peripherals/bcm2835_rng.h>

void bcm2835_start() {
  *BCM2835_RNG_STATUS = BCM2835_RNG_WARMUP_COUNT;
  *BCM2835_RNG_INT_MASK |= BCM2835_RNG_INT_OFF;
  *BCM2835_RNG_CTRL |= BCM2835_RNG_RBG_ENABLE; // Enable
}

u64 bcm2835_rand(u64 min, u64 max) {
  if (!((*BCM2835_RNG_CTRL) & BCM2835_RNG_RBG_ENABLE)) { // Is init needed?
    bcm2835_start();
  }
  while (((*BCM2835_RNG_STATUS) >> 24) < 2); // Wait until enough (2) words are ready
  return (((((u64) (*BCM2835_RNG_DATA)) << 32) | *BCM2835_RNG_DATA) % (max - min)) + min;
}
