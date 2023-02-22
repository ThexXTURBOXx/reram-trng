#pragma once

#include "common.h"

#include "base.h"

#define RNG_BASE (PBASE + 0x00104000)

#define RNG_CTRL ((u32*)(RNG_BASE + 0x00000000))
#define RNG_STATUS ((u32*)(RNG_BASE + 0x00000004))
#define RNG_DATA ((u32*)(RNG_BASE + 0x00000008))
#define RNG_FF_THRESHOLD ((u32*)(RNG_BASE + 0x0000000c))
#define RNG_INT_MASK ((u32*)(RNG_BASE + 0x00000010))
