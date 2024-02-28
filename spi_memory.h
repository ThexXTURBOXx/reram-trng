#pragma once

#include <circle/types.h>
#include "memory_config.h"

struct {
  u8 WriteProtectPin       : 1; // Protects writing to a status register
  u8 AutoPowerDownEnable   : 1;
  u8 LowPowerStandbyEnable : 1;
  u8 BlockProtectionBits   : 2; // Defines size of write protect block for the WRITE command
  u8 WriteEnableBit        : 1; // Indicates ReRAM Array and status register are writable
  u8 WriteInProgressBit    : 1; // Indicates ReRAM Array or status register are in writing process
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
  ReRAM_PD    = static_cast<u8>(0b10111001),
  ReRAM_UDPD  = static_cast<u8>(0b01111001),
  ReRAM_RES   = static_cast<u8>(0b10101011),
} typedef ReRamInstructions;
