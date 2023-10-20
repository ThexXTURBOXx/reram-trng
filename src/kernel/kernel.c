#include <common.h>
#include <mini_uart.h>
#include <printf.h>
#include <irq.h>
#include <gpio.h>
#include <memory_defines.h>
#include <timer.h>
#include <spi.h>
#include <spi_memory.h>
#include <mt19937ar.h>
#include <rng.h>
#include <utils.h>
#include <io.h>

#define BOOT_SIGNATURE 0xAA55

typedef struct PACKED {
  u8 head;
  u8 sector: 6;
  u8 cylinder_hi: 2;
  u8 cylinder_lo;
} chs_address;

typedef struct PACKED {
  u8 status;
  chs_address first_sector;
  u8 type;
  chs_address last_sector;
  u32 first_lba_sector;
  u32 num_sectors;
} partition_entry;

typedef struct PACKED {
  u8 bootCode[0x1BE];
  partition_entry partitions[4];
  u16 bootSignature;
} master_boot_record;

void mt_rng_test() {
  while (true) {
    int addr = (int) genrand_range(0, 512);
    int num1 = (int) genrand_range(0, 256);
    int num2 = (int) genrand_range(0, 256);
    printf("%d,%d,%d\n", addr, num1, num2);
  }
}

void internal_rng_test() {
  while (true) {
    int addr = (int) rand(0, 512);
    int num1 = (int) rand(0, 256);
    int num2 = (int) rand(0, 256);
    printf("%d,%d,%d\n", addr, num1, num2);
  }
}

void test_adr(const u32 adr) {
  u8 val = (u8) rand(0, 256);
  mem_write(adr, val);
  wip_polling_cycles();
  u8 value = 0;
  mem_read(adr, &value);
  if (value != val)
    printf("%d: 0x%x -> 0x%x\n", adr, val, value);
}

int write_latency_puf_test() {
  u8 num1 = 0, num2 = 0;
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
  u8 num1 = 0, num2 = 0;
  printf("From;To;Cell;WIP_Polling\n");
  do {
    u8 num1Arr[128] = {[0 ... 127] = num1};
    do {
      u8 num2Arr[128] = {[0 ... 127] = num2};
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

  u8 value;
  for (int i = 0; i < 512; ++i) {
    // Read out all values
    mem_read(i, &value);
    printf("%d: 0x%x\n", i, value);
  }
}

void latency_rowhammer_test() {
  const int toTest = 512;
  const int tries = 500;
  u64 lats[toTest];
  u64 lat;

  // Average latency before hammer
  printf("\n");
  for (int i = 0; i < toTest; ++i) {
    lats[i] = 0;
    for (int j = 0; j < tries; ++j) {
      mem_write(i, (int) rand(0, 256));
      lats[i] += wip_polling_cycles();
    }
    printf("%d: %d\n", i, lats[i]);
  }

  printf("\n\nHAMMER\n\n");

  // Hammer for some amount of time to "pre-heat"
  int addr = 169;
  for (int i = 0; i < 100000; ++i) {
    mem_write(addr, (int) rand(0, 256));
    wip_polling_cycles();
  }

  // Real hammer attack
  printf("\nAddress: Latency | Diff\n");
  for (int i = 0; i < toTest; ++i) {
    lat = 0;
    for (int j = 0; j < tries; ++j) {
      // Hammer some more
      for (int k = 0; k < 1000; ++k) {
        mem_write(addr, (int) rand(0, 256));
        wip_polling_cycles();
      }
      // Test latency of attack address
      mem_write(i, (int) rand(0, 256));
      lat += wip_polling_cycles();
    }
    printf("%d: %d | %d\n", i, lat, lats[i] - lat);
  }
}

u64 random_write_latency() {
  // These could also be fixed

  // Use HW RNG as "seed"
  /*int addr = (int) rand(0, 512);
  int num1 = (int) rand(0, 256);
  int num2 = (int) rand(0, 256);*/

  // Use MT19937AR as "seed"
  int addr = (int) genrand_range(0, 512);
  int num1 = (int) genrand_range(0, 256);
  int num2 = (int) genrand_range(0, 256);

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
  int totalToGenerate = 500000;
  int toGenerate = totalToGenerate;
  int totalGenerated = 0;
  u64 start = timer_get_ticks();
  u64 blockStart = start;
  int blockGenerated = toGenerate;
  while (toGenerate) {
    // Very basic implementation of von Neumann extractor
    totalGenerated += 2;
    bool bit1 = write_latency_random_bit();
    bool bit2 = write_latency_random_bit();
    if (bit1 == bit2) continue;
    // For more debug information:
    if (toGenerate % 10000 == 0) {
      if (toGenerate < totalToGenerate) {
        printf("\n%lld µs, %d\n", time_from(blockStart), totalGenerated - blockGenerated);
        blockStart = timer_get_ticks();
      }
      blockGenerated = totalGenerated;
    }
    printf("%d", bit1);
    --toGenerate;
  }
  printf("\n\nTime needed: %lld µs\n", time_from(start));
  printf("Total bits generated: %d\n\n", totalGenerated);
}

void kernel_main() {
  uart_init();
  init_printf(0, io_device_find("muart"));
  printf("\nRaspberry Pi Bare Metal OS Initializing...\n");

  irq_init_vectors();
  enable_interrupt_controller();
  irq_enable();
  timer_init();
  init_genrand(0);

#if RPI_VERSION == 3
#if RPI_BPLUS
  printf("\tBoard: Raspberry Pi 3B+\n");
#else
  printf("\tBoard: Raspberry Pi 3\n");
#endif
#elif RPI_VERSION == 4
  printf("\tBoard: Raspberry Pi 4\n");
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

  /*for (u32 i = 0; i < MEM_SIZE_ADR; ++i) {
    if (i % 1000 == 0) printf("%d\n", i);
    for (int j = 0; j < 100; ++j)
      test_adr(i);
  }*/

  //latency_rowhammer_test();

  //for (int i = 0; i < 8192; ++i)
  //  printf("%d\n", random_write_latency());

  printf("Shutting down...\n");

  while (1) {
    //uart_send(uart_recv());
    /*mem_write(0, 0xaa);
    for (int i = 0; i < 1024; ++i) {
        MemoryStatusRegister reg;
        read_status_register(&reg);
        printf("%d", reg.write_in_progress_bit);
    }
    printf("==========\n\n");
    timer_sleep(2000);*/
  }
}
