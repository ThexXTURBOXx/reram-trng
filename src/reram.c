#include "timer.h"
#include "reram.h"
#include "gpio.h"
#include "printf.h"
#include "spi.h"

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
                                   (statusRegister & 12) >> 3,
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
    u64 cycles = 0;
    u64 endTS = 0;
    u64 startTS = timer_get_ticks();

    do {
        cycles++;
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
    for (u64 cycles = 1;; cycles++) {
        MemoryStatusRegister statusRegister;
        read_status_register(&statusRegister);
        if (!statusRegister.write_in_progress_bit) {
            return cycles;
        }
    }
}

void reram_write(const u32 adr, u8 value) {
    u8 write_data[] = {ReRAM_WR, ((adr >> 8) & 0xFF), ((adr >> 0) & 0xFF), value};
    set_write_enable_latch(false);
    set_write_enable();
    spi_send(0, write_data, sizeof(write_data));
    reset_write_enable();
}

void reram_read(const u32 adr, u8 *ret) {
    u8 read_data[] = {ReRAM_READ, ((adr >> 8) & 0xFF), ((adr >> 0) & 0xFF)};
    u8 ret_val;
    spi_send_recv(0, read_data, sizeof(read_data), &ret_val, sizeof(ret_val));
    *ret = ret_val;
}
