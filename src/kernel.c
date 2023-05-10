#include "common.h"
#include "mini_uart.h"
#include "printf.h"
#include "irq.h"
#include "gpio.h"
#include "memory_defines.h"
#include "timer.h"
#include "spi.h"
#include "spi_memory.h"
#include "rng.h"

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

int write_latency_puf_test() {
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

int write_latency_puf_test_bulk_write() {
  uint8_t num1 = 0, num2 = 0;
  printf("From;To;Cell;WIP_Polling\n");
  do {
    uint8_t num1Arr[128] = {[0 ... 127] = num1};
    do {
      uint8_t num2Arr[128] = {[0 ... 127] = num2};
      for (int addrCtr = 0; addrCtr < 512; addrCtr++) {
        //for (int try = 0; try < 512; try++) {
        // Write first value
        mem_write_values(addrCtr, 128, num1Arr);

        // Wait until the WEL latch turns reset
        wip_polling_cycles();

        // Overwrite value
        mem_write_values(addrCtr, 128, num2Arr);

        // Measure write latency
        u64 write_latency = wip_polling_cycles();

        printf("%d;%d;%d;%d\n", num1, num2, addrCtr, write_latency);
        //}
      }
    } while (num2++ != 255);
  } while (num1++ != 255);
  return 0;
}

void rowhammer_test() {
  // Initialise all values to 0
  for (int i = 0; i < 512; ++i) {
    mem_write(i, 0);
    // Wait until the WEL latch turns reset
    wip_polling_cycles();
  }

  printf("\n");
  // Address to test
  int addr = 169;
  for (int i = 0; i < 1000000; ++i) {
    if (i % 10000 == 0) printf("\r%d", i);
    // Write random value
    mem_write(addr, (int) rand(0, 256));
    // Wait until the WEL latch turns reset
    //wip_polling_cycles();
  }
  printf("\n");

  uint8_t value;
  for (int i = 0; i < 512; ++i) {
    // Read out all values
    mem_read(i, &value);
    printf("%d: 0x%x\n", i, value);
  }
}

u64 random_write_latency() {
  // Use other RNG as seed
  int addr = (int) rand(0, 512);
  int num1 = (int) rand(0, 256);
  int num2 = (int) rand(0, 256);

  // Write first value
  mem_write(addr, num1);

  // Wait until the WEL latch turns reset
  wip_polling_cycles();

  // Overwrite value
  mem_write(addr, num2);

  // Measure write latency
  u64 write_latency = wip_polling_cycles();

  // This is the rather random latency
  return write_latency;
}

bool write_latency_random_bit() {
  // Extract "random" LSB
  return (bool) (random_write_latency() & 1);
}

void write_latency_rng_test() {
  int toGenerate = 500000;
  int totalGenerated = 0;
  u64 start = timer_get_ticks();
  u64 blockStart = start;
  int blockGenerated = toGenerate;
  while (toGenerate) {
    // Very basic implementation of von Neumann extractor
    ++totalGenerated;
    bool bit1 = write_latency_random_bit();
    bool bit2 = write_latency_random_bit();
    if (bit1 == bit2) continue;
    // For more debug information:
    /*if (toGenerate % 10000 == 0 && totalGenerated != 0) {
      printf("\n%ld µs, %d\n", time_from(blockStart), totalGenerated - blockGenerated);
      blockStart = timer_get_ticks();
      blockGenerated = totalGenerated;
    }*/
    printf("%d", bit1);
    --toGenerate;
  }
  printf("\n\nTime needed: %ld µs\n", time_from(start));
  printf("Total bits generated: %d\n\n", 2 * totalGenerated);
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

  printf("Expecting Memory module %s\n", MEM_NAME);

  printf("Testing Memory...\n");

  write_latency_rng_test();
  //for (int i = 0; i < 8192; ++i)
  //  printf("%d\n", random_write_latency());

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
