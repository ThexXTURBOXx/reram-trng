#include <circle/types.h>
#include "memory_config.h"

#ifndef CIRCLE_SPI_MEMORY_H
#define CIRCLE_SPI_MEMORY_H

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
  ReRAM_WRSR = (u8) 0b00000001,  // Write Status Register
  ReRAM_WR = (u8) 0b00000010,    // Write Memory
  ReRAM_READ = (u8) 0b00000011,  // Read Memory
  ReRAM_FREAD = (u8) 0b00001011, // Fast Read Memory
  ReRAM_WRDI = (u8) 0b00000100,  // Write Disable
  ReRAM_RDSR = (u8) 0b00000101,  // Read Status Register
  ReRAM_WREN = (u8) 0b00000110,  // Write Enable
  ReRAM_PERS = (u8) 0b01000010,  // Page Erase
  ReRAM_CERS = (u8) 0b01100000,  // Chip Erase
  ReRam_PD = (u8) 0b10111001,    // Power Down
  ReRAM_UDPD = (u8) 0b01111001,  // Ultra Deep Power Down
  ReRAM_RES = (u8) 0b10101011,   // Resume From Power Down
} typedef ReRamInstructions;

#endif //CIRCLE_SPI_MEMORY_H
