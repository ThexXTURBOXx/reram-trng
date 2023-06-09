#include <common.h>
#include <mini_uart.h>
#include <printf.h>
#include <irq.h>
#include <gpio.h>
#include <memory_defines.h>
#include <timer.h>
#include <spi.h>
#include <spi_memory.h>
#include <rng.h>
#include <utils.h>
#include <io.h>
#include <peripherals/emmc.h>

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
  // Use other RNG as "seed"
  // These could also be fixed
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
    if (toGenerate % 10000 == 0 && totalGenerated != 0) {
      printf("\n%ld µs, %d\n", time_from(blockStart), totalGenerated - blockGenerated);
      blockStart = timer_get_ticks();
      blockGenerated = totalGenerated;
    }
    printf("%d", bit1);
    --toGenerate;
  }
  printf("\n\nTime needed: %ld µs\n", time_from(start));
  printf("Total bits generated: %d\n\n", 2 * totalGenerated);
}

void kernel_main() {
  uart_init();
  init_printf(0, io_device_find("muart"));
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

  // We don't need memory stuff
  /*
#if INIT_MMU == 1
  printf("Initialized MMU\n");
#endif

  if (!emmc_init()) {
    printf("FAILED TO INIT eMMC\n");
    return;
  }

  printf("eMMC Disk initialized\n");

  master_boot_record mbr;

  io_device *disk = io_device_find("disk");

  int r = disk->read(disk, &mbr, sizeof(mbr));

  printf("Read disk returned: %d\n", r);

  if (mbr.bootSignature != BOOT_SIGNATURE) {
    printf("BAD BOOT SIGNATURE: %X\n", mbr.bootSignature);
  }

  for (int i = 0; i < 4; i++) {
    if (mbr.partitions[i].type == 0) {
      break;
    }

    printf("Partition %d:\n", i);
    printf("\t Type: %d\n", mbr.partitions[i].type);
    printf("\t NumSecs: %d\n", mbr.partitions[i].num_sectors);
    printf("\t Status: %d\n", mbr.partitions[i].status);
    printf("\t Start: %X\n", mbr.partitions[i].first_lba_sector);
  }

  timer_sleep(15000);

  void *p1 = get_free_pages(10);
  void *p2 = get_free_pages(4);
  void *p3 = allocate_memory(20 * 4096 + 1);

  free_memory(p1);
  free_memory(p2);
  free_memory(p3);*/

  printf("\nException Level: %d\n", get_el());

  printf("Sleeping 200 ms...\n");
  timer_sleep(200);

  // We don't need I2C here...
  /*printf("Initializing I2C...\n");
  i2c_init();

  for (u8 i=0x20; i<0x30; i++) {
      if (i2c_send(i, &i, 1) == I2CS_SUCCESS) {
          //we know there is an i2c device here now.
          printf("Found device at address 0x%X\n", i);
      }
  }*/

  printf("Initializing SPI...\n");
  spi_init();
  gpio_pin_set_func(25, GFOutput);

  // We don't need mailbox, I2C and video...
  /*printf("MAILBOX:\n");

  printf("CORE CLOCK: %d\n", mailbox_clock_rate(CT_CORE));
  printf("EMMC CLOCK: %d\n", mailbox_clock_rate(CT_EMMC));
  printf("UART CLOCK: %d\n", mailbox_clock_rate(CT_UART));
  printf("ARM  CLOCK: %d\n", mailbox_clock_rate(CT_ARM));

  printf("I2C POWER STATE:\n");

  for (int i=0; i<3; i++) {
      bool on = mailbox_power_check(i);

      printf("POWER DOMAIN STATUS FOR %d = %d\n", i, on);
  }

  //timer_sleep(2000);

  for (int i=0; i<3; i++) {
      u32 on = 1;
      mailbox_generic_command(RPI_FIRMWARE_SET_DOMAIN_STATE, i, &on);

      printf("SET POWER DOMAIN STATUS FOR %d = %d\n", i, on);
  }

  //timer_sleep(1000);

  for (int i=0; i<3; i++) {
      bool on = mailbox_power_check(i);

      printf("POWER DOMAIN STATUS FOR %d = %d\n", i, on);
  }

  u32 max_temp = 0;

  mailbox_generic_command(RPI_FIRMWARE_GET_MAX_TEMPERATURE, 0, &max_temp);

  //Do video...
  video_init();

  printf("NO DMA...\n");
  video_set_dma(false);

  printf("Resolution 1900x1200\n");
  video_set_resolution(1900, 1200, 32);

  printf("Resolution 1024x768\n");
  video_set_resolution(1024, 768, 32);

  printf("Resolution 800x600\n");
  video_set_resolution(800, 600, 32);

  printf("Resolution 1900x1200\n");
  video_set_resolution(1900, 1200, 8);

  printf("Resolution 1024x768\n");
  video_set_resolution(1024, 768, 8);

  printf("Resolution 800x600\n");
  video_set_resolution(800, 600, 8);

  printf("YES DMA...\n");
  video_set_dma(true);

  printf("Resolution 1900x1200\n");
  video_set_resolution(1900, 1200, 32);

  printf("Resolution 1024x768\n");
  video_set_resolution(1024, 768, 32);

  printf("Resolution 800x600\n");
  video_set_resolution(800, 600, 32);

  printf("Resolution 1900x1200\n");
  video_set_resolution(1900, 1200, 8);

  printf("Resolution 1024x768\n");
  video_set_resolution(1024, 768, 8);

  printf("Resolution 800x600\n");
  video_set_resolution(800, 600, 8);

  while(1) {
      u32 cur_temp = 0;

      mailbox_generic_command(RPI_FIRMWARE_GET_TEMPERATURE, 0, &cur_temp);

      printf("Cur temp: %dC MAX: %dC\n", cur_temp / 1000, max_temp / 1000);

      timer_sleep(1000);
  }*/

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