#include "spi.h"
#include "peripherals/spi.h"
#include "gpio.h"
#include "printf.h"

void spi_init() {
  // A divisor of 512 yields ~800 MHz whilst 256 would yield
  // around ~1.6 GHz, which is too much for the poor little Adesto :(
  REGS_SPI0->clock = 512;

  // Init GPIO pins
  gpio_pin_set_func(7, GFAlt0);  //CS1
  gpio_pin_set_func(8, GFAlt0);  //CS0
  gpio_pin_set_func(9, GFAlt0);  //MISO
  gpio_pin_set_func(10, GFAlt0); //MOSI
  gpio_pin_set_func(11, GFAlt0); //SCLK
  gpio_pin_enable(7);
  gpio_pin_enable(8);
  gpio_pin_enable(9);
  gpio_pin_enable(10);
  gpio_pin_enable(11);

  // Clear buffers
  REGS_SPI0->cs = CS_CLEAR_RX | CS_CLEAR_TX;
}

/**
 * First sends data and then receives data
 * Yes, SPI can be full-duplex, but the ReRam only needs Half-duplex mode...
 */
void spi_send_recv(u8 chip_select, u8 *sbuffer, u32 ssize, u8 *rbuffer, u32 rsize) {
  u32 total_len = ssize + rsize;
  REGS_SPI0->data_length = total_len;
  REGS_SPI0->cs = (REGS_SPI0->cs & ~CS_CS) | (chip_select << CS_CS__SHIFT) | CS_TA
      | CS_CLEAR_TX | CS_CLEAR_RX;

  u32 read_count = 0;
  u32 write_count = 0;
  while (write_count < total_len || read_count < total_len) {
    while (write_count < total_len && REGS_SPI0->cs & CS_TXD) {
      if (write_count < ssize) {
        REGS_SPI0->fifo = *sbuffer++;
      } else {
        REGS_SPI0->fifo = 0;
      }
      write_count++;
    }

    while (read_count < total_len && REGS_SPI0->cs & CS_RXD) {
      if (read_count >= ssize) {
        *rbuffer++ = REGS_SPI0->fifo;
      } else {
        (void) REGS_SPI0->fifo;
      }
      read_count++;
    }
  }

  while (!(REGS_SPI0->cs & CS_DONE)) {
    while (REGS_SPI0->cs & CS_RXD) {
      // Shouldn't happen (hopefully!)...
      printf("Left Over: %x\n", REGS_SPI0->fifo);
    }
  }

  REGS_SPI0->cs = (REGS_SPI0->cs & ~CS_TA);
}

void spi_send(u8 chip_select, u8 *data, u32 size) {
  spi_send_recv(chip_select, data, size, 0, 0);
}

void spi_recv(u8 chip_select, u8 *data, u32 size) {
  spi_send_recv(chip_select, 0, 0, data, size);
}
