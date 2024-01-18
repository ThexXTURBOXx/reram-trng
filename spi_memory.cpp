#include "kernel.h"
#include "spi_memory.h"

void CKernel::SetWriteEnable() {
  m_WEPin.Write(LOW);
}

void CKernel::ResetWriteEnable() {
  m_WEPin.Write(HIGH);
}

MemoryStatusRegister CKernel::ParseStatusRegister(const u8 statusRegister) {
  return (MemoryStatusRegister){
    static_cast<u8>((statusRegister & 0b10000000) >> 7),
    static_cast<u8>((statusRegister & 0b01000000) >> 6),
    static_cast<u8>((statusRegister & 0b00100000) >> 5),
    static_cast<u8>((statusRegister & 0b00001100) >> 2),
    static_cast<u8>((statusRegister & 0b00000010) >> 1),
    static_cast<u8>((statusRegister & 0b00000001))
  };
}

void CKernel::ReadStatusRegister(MemoryStatusRegister* statusRegister) {
  constexpr u8 data[] = {ReRAM_RDSR, 0};
  u8 reg[] = {0, 0};
  constexpr int len = sizeof(data) / sizeof(u8);
  if (m_SPIMaster.WriteRead(SPI_CHIP_SELECT, &data, reg, len) != len) {
    m_Logger.Write(FromKernel, LogPanic, "SPI write error");
  }
  *statusRegister = ParseStatusRegister(reg[1]);
}

void CKernel::SetWriteEnableLatch(const bool check_register) {
  constexpr u8 data[] = {ReRAM_WREN};
  constexpr int data_len = sizeof(data) / sizeof(u8);
  // Only needed for WRSR: SetWriteEnable();
  if (m_SPIMaster.Write(SPI_CHIP_SELECT, data, data_len) != data_len) {
    m_Logger.Write(FromKernel, LogPanic, "SPI write error");
  }
  // Only needed for WRSR: ResetWriteEnable();

  if (check_register) {
    MemoryStatusRegister statusRegister;
    do {
      ReadStatusRegister(&statusRegister);
    } while (statusRegister.WriteInProgressBit != 1);
  }
}

MeasurementResult CKernel::WIPPollingCycles(u64& cycles, const int timeout) {
  MemoryStatusRegister statusRegister;
  for (u64 i = 1; timeout < 0 || i < static_cast<u64>(timeout); ++i) {
    ReadStatusRegister(&statusRegister);
    if (!statusRegister.WriteInProgressBit) {
      cycles = i;
      return Okay;
    }
  }
  return FailedTotally;
}

void CKernel::MemWrite(u32 adr, u8 value) {
#if MEM_ADR_SEND == 2
  u8 write_data[] = {
    ReRAM_WR,
    static_cast<u8>(adr >> 8 & 0xFF),
    static_cast<u8>(adr >> 0 & 0xFF),
    value
  };
#elif MEM_ADR_SEND == 3
  u8 write_data[] = {
    ReRAM_WR,
    static_cast<u8>(adr >> 16 & 0xFF),
    static_cast<u8>(adr >> 8 & 0xFF),
    static_cast<u8>(adr >> 0 & 0xFF),
    value
  };
#endif
  constexpr int write_len = sizeof(write_data) / sizeof(u8);

  SetWriteEnableLatch(false);
  // Only needed for WRSR: SetWriteEnable();
  if (m_SPIMaster.Write(SPI_CHIP_SELECT, write_data, write_len) != write_len) {
    m_Logger.Write(FromKernel, LogPanic, "SPI write error");
  }
  // Only needed for WRSR: ResetWriteEnable();
}

u8 CKernel::MemRead(u32 adr) {
#if MEM_ADR_SEND == 2
  u8 write_data[] = {
    ReRAM_READ,
    static_cast<u8>(adr >> 8 & 0xFF),
    static_cast<u8>(adr >> 0 & 0xFF),
    0
  };
#elif MEM_ADR_SEND == 3
  u8 write_data[] = {
    ReRAM_READ,
    static_cast<u8>(adr >> 16 & 0xFF),
    static_cast<u8>(adr >> 8 & 0xFF),
    static_cast<u8>(adr >> 0 & 0xFF),
    0
  };
#endif
  u8 read_data[1 + MEM_ADR_SEND + 1];

  constexpr int len = sizeof(write_data) / sizeof(u8);

  if (m_SPIMaster.WriteRead(SPI_CHIP_SELECT, write_data, read_data, len) != len) {
    m_Logger.Write(FromKernel, LogPanic, "SPI write error");
  }
  return read_data[1 + MEM_ADR_SEND];
}

MeasurementResult CKernel::MemWriteAndPoll(u64& cycles, const u32 adr, const u8 value, const int timeout) {
  MemWrite(adr, value);
  return WIPPollingCycles(cycles, timeout);
}
