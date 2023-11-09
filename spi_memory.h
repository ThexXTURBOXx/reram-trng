#ifndef CIRCLE_SPI_MEMORY_H
#define CIRCLE_SPI_MEMORY_H

#include <circle/types.h>
#include "memory_config.h"

struct {
  /** This register protects writing to a status register */
  u8 WriteProtectPin;
  /** This register protects writing to a status register */
  u8 AutoPowerDownEnable;
  u8 LowPowerStandbyEnable;
  /** This defines size of write protect block for the WRITE command  */
  u8 BlockProtectionBits;
  /** This indicates ReRAM Array and status register are writable. */
  u8 WriteEnableBit;
  /** This indicates ReRAM Array and status register are in writing process. */
  u8 WriteInProgressBit;
} typedef MemoryStatusRegister;

enum {
  ReRAM_WRSR  = static_cast<u8>(0b00000001),
  ReRAM_WR    = static_cast<u8>(0b00000010),
  ReRAM_READ  = static_cast<u8>(0b00000011),
  ReRAM_FREAD = static_cast<u8>(0b00001011),
  ReRAM_WRDI  = static_cast<u8>(0b00000100),
  ReRAM_RDSR  = static_cast<u8>(0b00000101),
  ReRAM_WREN  = static_cast<u8>(0b00000110),
  ReRAM_PERS  = static_cast<u8>(0b01000010),
  ReRAM_CERS  = static_cast<u8>(0b01100000),
  ReRam_PD    = static_cast<u8>(0b10111001),
  ReRAM_UDPD  = static_cast<u8>(0b01111001),
  ReRAM_RES   = static_cast<u8>(0b10101011),
} typedef ReRamInstructions;

#endif //CIRCLE_SPI_MEMORY_H
