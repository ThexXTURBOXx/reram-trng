#pragma once

#include "common.h"

#include "base.h"

#define BCM2835_RNG_BASE            (PBASE + 0x00104000)

#define BCM2835_RNG_CTRL            ((u32*)(BCM2835_RNG_BASE + 0x00000000))
#define BCM2835_RNG_STATUS          ((u32*)(BCM2835_RNG_BASE + 0x00000004))
#define BCM2835_RNG_DATA            ((u32*)(BCM2835_RNG_BASE + 0x00000008))
#define BCM2835_RNG_FF_THRESHOLD    ((u32*)(BCM2835_RNG_BASE + 0x0000000c))
#define BCM2835_RNG_INT_MASK        ((u32*)(BCM2835_RNG_BASE + 0x00000010))

#define BCM2835_RNG_RBG_ENABLE      0x1
#define BCM2835_RNG_INT_OFF         0x1
#define BCM2835_RNG_WARMUP_COUNT    0x40000
