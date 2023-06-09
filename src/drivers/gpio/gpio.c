#include <gpio.h>
#include <utils.h>

void gpio_pin_set_func(u8 pinNumber, GpioFunc func) {
  u8 bitStart = (pinNumber * 3) % 30;
  u8 reg = pinNumber / 10;

  u32 selector = REGS_GPIO->func_select[reg];
  selector &= ~(7 << bitStart);
  selector |= (func << bitStart);

  REGS_GPIO->func_select[reg] = selector;
}

void gpio_pin_enable(u8 pinNumber) {
  REGS_GPIO->pupd_enable = 0;
  delay(150);
  REGS_GPIO->pupd_enable_clocks[pinNumber / 32] = 1 << (pinNumber % 32);
  delay(150);
  REGS_GPIO->pupd_enable = 0;
  REGS_GPIO->pupd_enable_clocks[pinNumber / 32] = 0;
}

void gpio_set_pin(u8 pinNumber) {
  REGS_GPIO->output_set.data[pinNumber / 32] |= 1 << (pinNumber % 32);
}

void gpio_clear_pin(u8 pinNumber) {
  REGS_GPIO->output_clear.data[pinNumber / 32] |= 1 << (pinNumber % 32);
}

bool gpio_read_pin(u8 pinNumber) {
  return (REGS_GPIO->level.data[pinNumber / 32] >> (pinNumber % 32)) & 1;
}
