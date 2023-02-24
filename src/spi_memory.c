#include "timer.h"
#include "spi_memory.h"
#include "gpio.h"
#include "printf.h"
#include "spi.h"
#include "memory_defines.h"

void set_write_enable() {
  gpio_clear_pin(25);
}

void reset_write_enable() {
  gpio_set_pin(25);
}

void print_status_register(MemoryStatusRegister reg) {
  printf("WP_Enable: %d\n"
         "Auto_Power_Down_Enable: %d\n"
         "Low_Power_Standby_Enable: %d\n"
         "Block_Protection_Bit: %d\n"
         "Write_Enable_Bit: %d\n"
         "Write_In_Progress_Bit: %d\n",
         reg.wp_enable_Pin,
         reg.auto_power_down_enable,
         reg.low_power_standby_enable,
         reg.block_protection_bits,
         reg.write_enable_bit,
         reg.write_in_progress_bit);
}

MemoryStatusRegister parse_status_register(u8 statusRegister) {
  return (MemoryStatusRegister) {(statusRegister & 128) >> 7,
      (statusRegister & 64) >> 6,
      (statusRegister & 32) >> 5,
      (statusRegister & 12) >> 2,
      (statusRegister & 2) >> 1,
      (statusRegister & 1)};
}

void read_status_register(MemoryStatusRegister *statusRegister) {
  u8 data = ReRAM_RDSR;
  u8 reg = 0;
  spi_send_recv(0, &data, sizeof(data), &reg, sizeof(reg));
  *statusRegister = parse_status_register(reg);
}

void set_write_enable_latch(bool check_register) {
  u8 data = ReRAM_WREN;
  set_write_enable();
  spi_send(0, &data, sizeof(data));
  reset_write_enable();

  if (check_register) {
    MemoryStatusRegister statusRegister;
    do {
      read_status_register(&statusRegister);
    } while (statusRegister.write_in_progress_bit != 1);
  }
}

u64 wip_polling(u64 timeout) {
  u64 endTS = 0;
  u64 startTS = timer_get_ticks();

  do {
    MemoryStatusRegister statusRegister;
    read_status_register(&statusRegister);

    if (!statusRegister.write_in_progress_bit) {
      endTS = timer_get_ticks() - startTS;
      return endTS;
    }
    if (timeout)
      endTS = timer_get_ticks();
  } while (startTS + timeout > endTS);

  return endTS;
}

u64 wip_polling_cycles() {
  MemoryStatusRegister statusRegister;
  for (u64 cycles = 1;; ++cycles) {
    read_status_register(&statusRegister);
    if (!statusRegister.write_in_progress_bit) return cycles;
  }
}

void mem_write(const u32 adr, u8 value) {
#if MEM_ADR_SEND == 2
  u8 write_data[] = {ReRAM_WR, ((adr >> 8) & 0xFF), ((adr >> 0) & 0xFF), value};
#elif MEM_ADR_SEND == 3
  u8 write_data[] = {ReRAM_WR, ((adr >> 16) & 0xFF), ((adr >> 8) & 0xFF), ((adr >> 0) & 0xFF), value};
#endif
  set_write_enable_latch(false);
  set_write_enable();
  spi_send(0, write_data, sizeof(write_data));
  reset_write_enable();
}

void mem_write_values(u32 adr, u8 valuesLen, const u8 *values) {
#if MEM_ADR_SEND == 2
  u8 write_data[valuesLen + 3];
  write_data[0] = ReRAM_WR;
  write_data[1] = ((adr >> 8) & 0xFF);
  write_data[2] = ((adr >> 0) & 0xFF);
#elif MEM_ADR_SEND == 3
  u8 write_data[valuesLen + 4];
  write_data[0] = ReRAM_WR;
  write_data[1] = ((adr >> 16) & 0xFF);
  write_data[2] = ((adr >> 8) & 0xFF);
  write_data[3] = ((adr >> 0) & 0xFF);
#endif
  for (int i = 0; i < valuesLen; i++) {
    write_data[i + 1 + MEM_ADR_SEND] = values[i];
  }
  set_write_enable_latch(false);
  set_write_enable();
  spi_send(0, write_data, sizeof(write_data));
  reset_write_enable();
}

void mem_read(const u32 adr, u8 *ret) {
#if MEM_ADR_SEND == 2
  u8 read_data[] = {ReRAM_READ, ((adr >> 8) & 0xFF), ((adr >> 0) & 0xFF)};
#elif MEM_ADR_SEND == 3
  u8 read_data[] = {ReRAM_READ, ((adr >> 16) & 0xFF), ((adr >> 8) & 0xFF), ((adr >> 0) & 0xFF)};
#endif
  u8 ret_val;
  spi_send_recv(0, read_data, sizeof(read_data), &ret_val, sizeof(ret_val));
  *ret = ret_val;
}
