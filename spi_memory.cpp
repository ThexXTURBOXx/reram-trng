#include "kernel.h"
#include "spi_memory.h"

void CKernel::SetWriteEnable() {
  m_WEPin.Write(LOW);
}

void CKernel::ResetWriteEnable() {
  m_WEPin.Write(HIGH);
}

MemoryStatusRegister CKernel::ParseStatusRegister(u8 statusRegister) {
  return (MemoryStatusRegister) {
      static_cast<u8>((statusRegister & 128) >> 7),
      static_cast<u8>((statusRegister & 64) >> 6),
      static_cast<u8>((statusRegister & 32) >> 5),
      static_cast<u8>((statusRegister & 12) >> 2),
      static_cast<u8>((statusRegister & 2) >> 1),
      static_cast<u8>((statusRegister & 1))
  };
}

void CKernel::ReadStatusRegister(MemoryStatusRegister *statusRegister) {
  u8 data[] = {ReRAM_RDSR, 0};
  u8 reg[] = {0, 0};
  int len = sizeof(data);
  if (m_SPIMaster.WriteRead(SPI_CHIP_SELECT, &data, reg, len) != len) {
    CLogger::Get()->Write(FromKernel, LogPanic, "SPI write error");
  }
  *statusRegister = ParseStatusRegister(reg[1]);
}

void CKernel::SetWriteEnableLatch(bool check_register) {
  u8 data = ReRAM_WREN;
  int data_len = sizeof(data);
  // Only needed for WRSR: SetWriteEnable();
  if (m_SPIMaster.Write(SPI_CHIP_SELECT, &data, data_len) != data_len) {
    CLogger::Get()->Write(FromKernel, LogPanic, "SPI write error");
  }
  // Only needed for WRSR: ResetWriteEnable();

  if (check_register) {
    MemoryStatusRegister statusRegister;
    do {
      ReadStatusRegister(&statusRegister);
    } while (statusRegister.WriteInProgressBit != 1);
  }
}

u64 CKernel::WIPPollingCycles() {
  MemoryStatusRegister statusRegister;
  for (u64 cycles = 1;; ++cycles) {
    ReadStatusRegister(&statusRegister);
    if (!statusRegister.WriteInProgressBit) return cycles;
  }
}

void CKernel::MemWrite(u32 adr, u8 value) {
#if MEM_ADR_SEND == 2
  u8 write_data[] = {
      ReRAM_WR,
      static_cast<u8>((adr >> 8) & 0xFF),
      static_cast<u8>((adr >> 0) & 0xFF),
      value
  };
#elif MEM_ADR_SEND == 3
  u8 write_data[] = {
      ReRAM_WR,
      static_cast<u8>((adr >> 16) & 0xFF),
      static_cast<u8>((adr >> 8) & 0xFF),
      static_cast<u8>((adr >> 0) & 0xFF),
      value
  };
#endif
  int write_len = sizeof(write_data);

  SetWriteEnableLatch(false);
  // Only needed for WRSR: SetWriteEnable();
  if (m_SPIMaster.Write(SPI_CHIP_SELECT, write_data, write_len) != write_len) {
    CLogger::Get()->Write(FromKernel, LogPanic, "SPI write error");
  }
  // Only needed for WRSR: ResetWriteEnable();
}
