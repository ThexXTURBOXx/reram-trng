#pragma once

#include "common.h"

struct {
    u8 wp_enable_Pin;
    u8 auto_power_down_enable;
    u8 low_power_standby_enable;
    u8 block_protection_bits;
    u8 write_enable_bit;
    u8 write_in_progress_bit;
} typedef MemoryStatusRegister;

enum {
    ReRAM_WRSR = (u8) 0x01,  // Write Status Register
    ReRAM_WR = (u8) 0x02,    // Write Memory
    ReRAM_READ = (u8) 0x03,  // Read Memory
    ReRAM_FREAD = (u8) 0x0B, // Fast Read Memory
    ReRAM_WRDI = (u8) 0x04,  // Write Disable
    ReRAM_RDSR = (u8) 0x05,  // Read Status Register
    ReRAM_WREN = (u8) 0x06,  // Write Enable
    ReRAM_PERS = (u8) 0x42,  // Page Erase
    ReRAM_CERS = (u8) 0x60,  // Chip Erase
    ReRam_PD = (u8) 0xB9,    // Power Down
    ReRAM_UDPD = (u8) 0x79,  // Ultra Deep Power Down
    ReRAM_RES = (u8) 0xAB,   // Resume From Power Down
} typedef ReRamInstructions;

void set_write_enable();

void reset_write_enable();

void print_status_register(MemoryStatusRegister reg);

MemoryStatusRegister parse_status_register(u8 statusRegister);

void read_status_register(MemoryStatusRegister *statusRegister);

void set_write_enable_latch(bool check_register);

u64 wip_polling(u64 timeout);

u64 wip_polling_cycles();

void mem_write(u32 adr, u8 value);

void mem_read(u32 adr, u8 *ret);
