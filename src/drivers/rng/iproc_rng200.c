#include <common.h>
#include <iproc_rng200.h>
#include <peripherals/iproc_rng200.h>

void iproc_rng200_enable_set(bool enable) {
  u32 val;
  val = *IPROC_RNG200_CTRL;
  val &= ~IPROC_RNG200_CTRL_RNG_RBGEN_MASK;
  if (enable)
    val |= IPROC_RNG200_CTRL_RNG_RBGEN_ENABLE;
  *IPROC_RNG200_CTRL = val;
}

void iproc_rng200_restart() {
  u32 val;

  iproc_rng200_enable_set(false);

  /* Clear all interrupt status */
  *IPROC_RNG200_INT_STATUS = 0xFFFFFFFFUL;

  /* Reset RNG and RBG */
  val = *IPROC_RNG200_RBG_SOFT_RESET;
  val |= IPROC_RNG200_RBG_SOFT_RESET_SIGNAL;
  *IPROC_RNG200_RBG_SOFT_RESET = val;

  val = *IPROC_RNG200_SOFT_RESET;
  val |= IPROC_RNG200_SOFT_RESET_SIGNAL;
  *IPROC_RNG200_SOFT_RESET = val;

  val = *IPROC_RNG200_SOFT_RESET;
  val &= ~IPROC_RNG200_SOFT_RESET_SIGNAL;
  *IPROC_RNG200_SOFT_RESET = val;

  val = *IPROC_RNG200_RBG_SOFT_RESET;
  val &= ~IPROC_RNG200_RBG_SOFT_RESET_SIGNAL;
  *IPROC_RNG200_RBG_SOFT_RESET = val;

  iproc_rng200_enable_set(true);
}

u32 iproc_rng200_rand_read() {
  uint32_t status;
  while (true) {
    /* Is RNG sane? If not, reset it. */
    status = *IPROC_RNG200_INT_STATUS;
    if ((status & (IPROC_RNG200_INT_STATUS_MASTER_FAIL_LOCKOUT_IRQ_MASK |
        IPROC_RNG200_INT_STATUS_NIST_FAIL_IRQ_MASK)) != 0) {
      iproc_rng200_restart();
    }

    /* Is there a random number available? */
    if (((*IPROC_RNG200_FIFO_COUNT) & IPROC_RNG200_FIFO_COUNT_RNG_FIFO_COUNT_MASK) > 0) {
      return *IPROC_RNG200_FIFO_DATA;
    } else {
      /* Can wait, give others chance to run */
      //usleep(10);
    }
  }
}

u64 iproc_rng200_rand(u64 min, u64 max) {
  if (!((*IPROC_RNG200_CTRL) & IPROC_RNG200_CTRL_RNG_RBGEN_ENABLE)) { // Is init needed?
    iproc_rng200_enable_set(true);
  }
  return (((((u64) iproc_rng200_rand_read()) << 32) | iproc_rng200_rand_read()) % (max - min)) + min;
}
