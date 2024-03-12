#pragma once

#include <circle/actled.h>
#include <circle/bcmrandom.h>
#include <circle/koptions.h>
#include <circle/devicenameservice.h>
#include <circle/serial.h>
#include <circle/exceptionhandler.h>
#include <circle/interrupt.h>
#include <circle/timer.h>
#include <circle/logger.h>
#include <circle/spimaster.h>
#include <circle/types.h>
#include <SDCard/emmc.h>
#include <fatfs/ff.h>
#include "spi_memory.h"

#define SPI_MASTER_DEVICE      0             // 0, 4, 5, 6 on Raspberry Pi 4; 0 otherwise
#define SPI_CPOL               0
#define SPI_CPHA               0
#define SPI_CHIP_SELECT        0             // 0 or 1, or 2 (for SPI1)

static constexpr char FromKernel[] = "kernel";

enum TShutdownMode {
  ShutdownNone,
  ShutdownHalt,
  ShutdownReboot
};

enum MeasurementResult {
  // Everything went fine
  Okay,
  // Was probably able to still write data
  FailedPartially,
  // Did not do anything
  FailedTotally
};

class CKernel {
public:
  CKernel();

  ~CKernel();

  bool Initialize();

  TShutdownMode Run();

  // Helper functions - TODO: Maybe move to own lib?

  static bool FileExists(const char* path);

  static CString GetFreeFile(const char* pattern);

  // Kernel functionality

  void IndicateStop(MeasurementResult);

  MeasurementResult RandomWriteLatency(u64& write_latency, int addr, int num1, int num2, int timeout = -1);

  MeasurementResult RandomWriteLatency(u64& write_latency, int timeout = -1);

  MeasurementResult WriteLatencyRandomBit(bool& bit, int timeout = -1);

  MeasurementResult ExtractSingleBit(bool& bit, int& totalGenerated, int tries = -1, int timeout = -1);

  MeasurementResult IsBurntOut(bool& burntOut, int addr, int writes = 10, int timeout = -1);

  /**
   * WARNING! THIS PERMANENTLY DAMAGES THE GIVEN CELL. USE CAREFULLY!
   */
  MeasurementResult BurnOut(int addr, int checkInterval = 1000, int timeout = -1);

  MeasurementResult DemoMode();

  MeasurementResult WriteLatencyRngTest();

  MeasurementResult WriteLatencyRngTest2();

  MeasurementResult BurnOutCells();

  // SPI Memory

  void SetWriteEnable();

  void ResetWriteEnable();

  static MemoryStatusRegister ParseStatusRegister(u8 statusRegister);

  void ReadStatusRegister(MemoryStatusRegister* statusRegister);

  void SetWriteEnableLatch(bool check_register);

  MeasurementResult WIPPollingCycles(u64& cycles, int timeout = -1);

  void MemWrite(u32 adr, u8 value);

  u8 MemRead(u32 adr);

  MeasurementResult MemWriteAndPoll(u64& cycles, u32 adr, u8 value, int timeout = -1);

private:
  // do not change this order
  CActLED m_ActLED;
  CKernelOptions m_Options;
  CDeviceNameService m_DeviceNameService;
  CSerialDevice m_Serial;
  CExceptionHandler m_ExceptionHandler;
  CInterruptSystem m_Interrupt;
  CTimer m_Timer;
  CLogger m_Logger;
  CSPIMaster m_SPIMaster;
  CBcmRandomNumberGenerator m_Random;
  CGPIOPin m_WEPin;
  CEMMCDevice m_EMMC;
  FATFS m_FileSystem;
};
