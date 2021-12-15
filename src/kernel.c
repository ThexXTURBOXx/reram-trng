#include "common.h"
#include "mini_uart.h"
#include "printf.h"
#include "irq.h"
#include "gpio.h"
#include "timer.h"
#include "spi.h"
#include "spi_memory.h"

void putc(void *p, char c) {
    if (c == '\n') {
        uart_send('\r');
    }
    uart_send(c);
}

u32 get_el();

void test_adr(const uint32_t adr) {
    mem_write(adr, 0xaa);
    wip_polling(0);
    uint8_t value = 0;
    mem_read(adr, &value);
    printf("Read: %d -> 0x%x\n", adr, value);
}

int adesto_test() {
    uint8_t num1 = 0, num2 = 0;
    printf("From;To;Cell;WIP_Polling\n");
    do {
        do {
            for (int addrCtr = 0; addrCtr < 512; addrCtr++) {
                //for (int try = 0; try < 512; try++) {
                // Write first value
                mem_write(addrCtr, num1);

                // Wait until the WEL latch turns reset
                wip_polling_cycles();

                // Overwrite value
                mem_write(addrCtr, num2);

                // Measure write latency
                u64 write_latency = wip_polling_cycles();

                printf("%d;%d;%d;%d\n", num1, num2, addrCtr, write_latency);
                //}
            }
        } while (num2++ != 255);
    } while (num1++ != 255);
    return 0;
}

void kernel_main() {
    uart_init();
    init_printf(0, putc);
    printf("\nRaspberry PI Bare Metal OS Initializing...\n");

    irq_init_vectors();
    enable_interrupt_controller();
    irq_enable();
    timer_init();

#if RPI_VERSION == 3
#if RPI_BPLUS
    printf("\tBoard: Raspberry PI 3B+\n");
#else
    printf("\tBoard: Raspberry PI 3\n");
#endif
#elif RPI_VERSION == 4
    printf("\tBoard: Raspberry PI 4\n");
#endif

    printf("\nException Level: %d\n", get_el());

    printf("Sleeping 200 ms...\n");
    timer_sleep(200);

    printf("Initializing SPI...\n");
    spi_init();
    gpio_pin_set_func(25, GFOutput);

    printf("Testing Memory...\n");

    adesto_test();

    printf("Shutting down...\n");

    while (1) {
        //uart_send(uart_recv());
        /*for (uint32_t i = 0; i < 512; i++) {
            test_adr(i);
            timer_sleep(100);
        }*/
        /*mem_write(0, 0xaa);
        for (int i = 0; i < 1024; i++) {
            MemoryStatusRegister reg;
            read_status_register(&reg);
            printf("%d", reg.write_in_progress_bit);
        }
        printf("==========\n\n");
        timer_sleep(2000);*/
    }
}
