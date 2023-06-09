#include <peripherals/timer.h>
#include <peripherals/irq.h>

const u32 interval_1 = CLOCKHZ;
u32 cur_val_1 = 0;

const u32 interval_3 = CLOCKHZ / 4;
u32 cur_val_3 = 0;

void timer_init() {
  cur_val_1 = REGS_TIMER->counter_lo;
  cur_val_1 += interval_1;
  REGS_TIMER->compare[1] = cur_val_1;

  cur_val_3 = REGS_TIMER->counter_lo;
  cur_val_3 += interval_3;
  REGS_TIMER->compare[3] = cur_val_3;
}

void handle_timer_1() {
  cur_val_1 += interval_1;
  REGS_TIMER->compare[1] = cur_val_1;
  REGS_TIMER->control_status |= SYS_TIMER_IRQ_1;

  //printf("Timer 1 received.\n");
}

void handle_timer_3() {
  cur_val_3 += interval_3;
  REGS_TIMER->compare[3] = cur_val_3;
  REGS_TIMER->control_status |= SYS_TIMER_IRQ_3;

  //printf("Timer 3 received.\n");
}

u64 timer_get_ticks() {
  u32 hi = REGS_TIMER->counter_hi;
  u32 lo = REGS_TIMER->counter_lo;

  // double check hi value didn't change after setting it...
  if (hi != REGS_TIMER->counter_hi) {
    hi = REGS_TIMER->counter_hi;
    lo = REGS_TIMER->counter_lo;
  }

  return ((u64) hi << 32) | lo;
}

/**
 * Sleep some milliseconds.
 */
void timer_sleep(u32 ms) {
  u64 end = timer_get_ticks() + (ms * 1000);
  while (timer_get_ticks() < end) {}
}

u64 time_from(u64 start) {
  return timer_get_ticks() - start;
}

u64 ticks_to_ms(u64 ticks) {
  return ticks / 1000;
}
