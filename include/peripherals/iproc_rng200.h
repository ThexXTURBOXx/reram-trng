#pragma once

#include "common.h"

#include "base.h"

#define IPROC_RNG200_BASE              (PBASE + 0x00104000)

#define IPROC_RNG200_CTRL              ((u32*)(IPROC_RNG200_BASE + 0x00000000))
#define IPROC_RNG200_SOFT_RESET        ((u32*)(IPROC_RNG200_BASE + 0x00000004))
#define IPROC_RNG200_RBG_SOFT_RESET    ((u32*)(IPROC_RNG200_BASE + 0x00000008))
#define IPROC_RNG200_INT_STATUS        ((u32*)(IPROC_RNG200_BASE + 0x00000018))
#define IPROC_RNG200_FIFO_DATA         ((u32*)(IPROC_RNG200_BASE + 0x00000020))
#define IPROC_RNG200_FIFO_COUNT        ((u32*)(IPROC_RNG200_BASE + 0x00000024))

#define IPROC_RNG200_CTRL_RNG_RBGEN_MASK                            0x00001FFF
#define IPROC_RNG200_CTRL_RNG_RBGEN_ENABLE                          0x00000001
#define IPROC_RNG200_SOFT_RESET_SIGNAL                              0x00000001
#define IPROC_RNG200_RBG_SOFT_RESET_SIGNAL                          0x00000001
#define IPROC_RNG200_INT_STATUS_MASTER_FAIL_LOCKOUT_IRQ_MASK        0x80000000
#define IPROC_RNG200_INT_STATUS_STARTUP_TRANSITIONS_MET_IRQ_MASK    0x00020000
#define IPROC_RNG200_INT_STATUS_NIST_FAIL_IRQ_MASK                  0x00000020
#define IPROC_RNG200_INT_STATUS_TOTAL_BITS_COUNT_IRQ_MASK           0x00000001
#define IPROC_RNG200_FIFO_COUNT_RNG_FIFO_COUNT_MASK                 0x000000FF
