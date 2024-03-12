//
// kernel.cpp
//
#include "kernel.h"

#include "mt19937ar.h"
#include <Properties/propertiesfatfsfile.h>

#define DRIVE        "SD:"
#define PARAMFILE    "/params.properties"

CKernel::CKernel()
  : m_Timer(&m_Interrupt),
    m_Logger(LogDebug, &m_Timer),
    m_SPIMaster(SPI_FREQ, SPI_CPOL, SPI_CPHA, SPI_MASTER_DEVICE),
    m_WEPin(25, GPIOModeOutput),
    m_EMMC(&m_Interrupt, &m_Timer, &m_ActLED),
    m_FileSystem() {}

CKernel::~CKernel() = default;

bool CKernel::Initialize() {
  bool bOK = TRUE;

  bOK = m_Serial.Initialize(115200);

  if (bOK) {
    CDevice* pTarget = m_DeviceNameService.GetDevice(m_Options.GetLogDevice(), FALSE);
    if (pTarget == nullptr) {
      pTarget = &m_Serial;
    }
    bOK = m_Logger.Initialize(pTarget);
  }

  if (bOK) {
    bOK = m_Interrupt.Initialize();
  }

  if (bOK) {
    bOK = m_Timer.Initialize();
  }

  if (bOK) {
    bOK = m_SPIMaster.Initialize();
  }

  if (bOK) {
    bOK = m_EMMC.Initialize();
  }

  return bOK;
}

TShutdownMode CKernel::Run() {
  m_Logger.Write(FromKernel, LogNotice, "Compile time: " __DATE__ " " __TIME__);
  m_Logger.Write(FromKernel, LogNotice, "Memory: %s, SPI Frequency: %lld Hz", MEM_NAME, SPI_FREQ);

  // Do dummy measurement
  int raw = 0;
  bool bit;
  MeasurementResult result = ExtractSingleBit(bit, raw, 1000, 1000);
  if (result != Okay) {
    m_Logger.Write(FromKernel, LogNotice, "Failed to generate single bit... Shutting down...");
    IndicateStop(result);
    return ShutdownNone;
  }

  // Successfully generated single bit
  m_Logger.Write(FromKernel, LogNotice, "Successfully generated bit %d with %d raw bits!", bit, raw);
  m_ActLED.Blink(5, 100, 100);

  // Mount file system
  if (f_mount(&m_FileSystem, DRIVE, 1) != FR_OK) {
    m_Logger.Write(FromKernel, LogPanic, "Cannot mount drive: %s", DRIVE);
    IndicateStop(FailedTotally);
    return ShutdownNone;
  }

  // Read param file
  CPropertiesFatFsFile Properties(DRIVE PARAMFILE, &m_FileSystem);
  if (!Properties.Load()) {
    m_Logger.Write(FromKernel, LogPanic, "Error loading properties from %s (line %u)",
                   PARAMFILE, Properties.GetErrorLine());
    IndicateStop(FailedTotally);
    return ShutdownNone;
  }

  // Read selected mode
  const char* cMode = Properties.GetString("mode", "trng");
  const CString mode(cMode);
  m_Logger.Write(FromKernel, LogNotice, "Selected mode: %s", cMode);

  // Run selected mode
  if (mode.Compare("demo") == 0)
    result = DemoMode();
  else if (mode.Compare("raw") == 0)
    result = WriteLatencyRngTest2();
  else if (mode.Compare("burnout") == 0)
    result = BurnOutCells();
  else
    result = WriteLatencyRngTest();

  // Shutdown
  IndicateStop(result);
  return ShutdownNone;
}

bool CKernel::FileExists(const char* path) {
  return f_stat(path, nullptr) == FR_OK;
}

CString CKernel::GetFreeFile(const char* pattern) {
  CString Msg;
  for (int i = 0; ; ++i) {
    Msg.Format(pattern, i);
    if (!FileExists(Msg)) return Msg;
  }
}

void CKernel::IndicateStop(const MeasurementResult result) {
  switch (result) {
  case Okay:
    m_ActLED.Blink(1000000000); // Will pretty much never stop
    break;
  case FailedPartially:
    m_ActLED.Blink(1000000000, 2000, 2000);
    break;
  case FailedTotally:
    m_ActLED.On();
    break;
  }
}

MeasurementResult CKernel::RandomWriteLatency(u64& write_latency, const int addr, const int num1, const int num2,
                                              const int timeout) {
  // Write first value
  const MeasurementResult result = MemWriteAndPoll(write_latency, addr, num1, timeout);
  if (result != Okay) return result;

  // Overwrite value; write_latency should be rather random now
  return MemWriteAndPoll(write_latency, addr, num2, timeout);
}

MeasurementResult CKernel::RandomWriteLatency(u64& write_latency, const int timeout) {
  // These could also be fixed

  // Use HW RNG as "seed"
  /*const int addr = static_cast<int>(m_Random.GetNumber() % MEM_SIZE_ADR);
  const int num1 = static_cast<int>(m_Random.GetNumber() % 256);
  const int num2 = static_cast<int>(m_Random.GetNumber() % 256);*/

  // Use MT19937AR as "seed"
  const int addr = static_cast<int>(genrand_range(0, MEM_SIZE_ADR));
  const int num1 = static_cast<int>(genrand_range(0, 256));
  const int num2 = static_cast<int>(genrand_range(0, 256));

  return RandomWriteLatency(write_latency, addr, num1, num2, timeout);
}

MeasurementResult CKernel::WriteLatencyRandomBit(bool& bit, const int timeout) {
  // Extract "random" LSB
  u64 write_latency;
  const MeasurementResult result = RandomWriteLatency(write_latency, timeout);
  bit = static_cast<bool>(write_latency & 1);
  return result;
}

MeasurementResult CKernel::ExtractSingleBit(bool& bit, int& totalGenerated, int tries, const int timeout) {
  bool bit1, bit2;
  while (tries < 0 || tries-- > 0) {
    // Very basic implementation of von Neumann extractor
    MeasurementResult result = WriteLatencyRandomBit(bit1, timeout);
    if (result != Okay) continue;
    result = WriteLatencyRandomBit(bit2, timeout);
    if (result != Okay) continue;
    totalGenerated += 2;
    if (bit1 != bit2) {
      bit = bit1;
      return Okay;
    }
  }
  return FailedTotally;
}

MeasurementResult CKernel::IsBurntOut(bool& burntOut, const int addr, const int writes, const int timeout) {
  burntOut = false;
  u64 temp;
  for (int i = 0; i < writes; ++i) {
    const u8 expected = m_Random.GetNumber() % 256;
    const MeasurementResult result = MemWriteAndPoll(temp, addr, expected, timeout);
    if (result != Okay) return result;
    if (expected != MemRead(addr)) {
      burntOut = true;
      break;
    }
  }
  return Okay;
}

MeasurementResult CKernel::BurnOut(const int addr, const int checkInterval, const int timeout) {
  u64 temp;
  MeasurementResult result;
  bool burntOut;
  for (int i = 0; ; ++i) {
    result = MemWriteAndPoll(temp, addr, m_Random.GetNumber() % 256, timeout);
    if (result != Okay) return result;
    if (i % checkInterval == 0) {
      result = IsBurntOut(burntOut, addr, 10, timeout);
      if (result != Okay) return result;
      if (burntOut) break;
    }
  }
  return result;
}

MeasurementResult CKernel::DemoMode() {
  MeasurementResult result = Okay;

  bool bit;
  while (result == Okay) {
    result = WriteLatencyRandomBit(bit);
    m_Timer.MsDelay(5 * 1000);
  }

  return result;
}

MeasurementResult CKernel::WriteLatencyRngTest() {
  MeasurementResult result = Okay;

#define FILENAME_BITS MEM_NAME_SIMPLE "_%d_bits.log"
  const CString fileNameBits = GetFreeFile(DRIVE FILENAME_BITS);
  const char* cFileNameBits = fileNameBits;
  m_Logger.Write(FromKernel, LogNotice, "Choosing bits file %s", cFileNameBits);
#define FILENAME_DEBUG MEM_NAME_SIMPLE "_%d_debug.log"
  const CString fileNameDebug = GetFreeFile(DRIVE FILENAME_DEBUG);
  const char* cFileNameDebug = fileNameDebug;
  m_Logger.Write(FromKernel, LogNotice, "Choosing debug file %s", cFileNameDebug);

  FIL file;
  int idxDebug = 0;
  u64 newUptime;

  constexpr int totalToGenerate = 500000;
  constexpr int debugSteps = 10000;

  char generated[totalToGenerate];
  u64 debugTimes[totalToGenerate / debugSteps];
  int debugBits[totalToGenerate / debugSteps];

  bool bit;
  int toGenerate = totalToGenerate;
  int totalGenerated = 0;
  const u64 start = CTimer::GetClockTicks64();
  u64 blockStart = start;
  int blockGenerated = toGenerate;
  while (toGenerate > 0) {
    ExtractSingleBit(bit, totalGenerated);
    // For more debug information:
    if (toGenerate % debugSteps == 0) {
      if (toGenerate < totalToGenerate) {
        newUptime = CTimer::GetClockTicks64();
        debugTimes[idxDebug] = newUptime - blockStart;
        debugBits[idxDebug] = totalGenerated - blockGenerated;
        m_Logger.Write(FromKernel, LogNotice, "%lld µs, %d",
                       debugTimes[idxDebug], debugBits[idxDebug]);
        blockStart = newUptime;
        ++idxDebug;
      }
      blockGenerated = totalGenerated;
    }
    //m_Logger.Write(FromMeasure, LogNotice, "%d", bit1);
    generated[totalToGenerate - toGenerate] = static_cast<char>('0' + bit);
    --toGenerate;
  }

  newUptime = CTimer::GetClockTicks64();
  debugTimes[idxDebug] = newUptime - blockStart;
  debugBits[idxDebug] = totalGenerated - blockGenerated;
  m_Logger.Write(FromKernel, LogNotice, "%lld µs, %d",
                 debugTimes[idxDebug], debugBits[idxDebug]);

  m_Logger.Write(FromKernel, LogNotice, "Time needed: %lld µs", newUptime - start);
  m_Logger.Write(FromKernel, LogNotice, "Total bits generated: %d\n", totalGenerated);

  FRESULT Result = f_open(&file, fileNameBits, FA_WRITE | FA_CREATE_ALWAYS);
  if (Result != FR_OK) {
    m_Logger.Write(FromKernel, LogPanic, "Cannot create file: %s (%d)", cFileNameBits, Result);
    result = FailedTotally;
  }
  unsigned nBytesWritten;
  Result = f_write(&file, generated, totalToGenerate, &nBytesWritten);
  if (Result != FR_OK || nBytesWritten != totalToGenerate) {
    m_Logger.Write(FromKernel, LogError, "Write error (%d)", Result);
    result = FailedTotally;
  }
  Result = f_close(&file);
  if (Result == FR_OK) {
    m_Logger.Write(FromKernel, LogNotice, "Successfully written bits to %s!", cFileNameBits);
  } else {
    m_Logger.Write(FromKernel, LogPanic, "Cannot close bits file (%d)", Result);
    result = FailedPartially;
  }

  Result = f_open(&file, fileNameDebug, FA_WRITE | FA_CREATE_ALWAYS);
  if (Result != FR_OK) {
    m_Logger.Write(FromKernel, LogPanic, "Cannot create file: %s (%d)", cFileNameDebug, Result);
    result = FailedPartially;
  }
  CString Msg;
  for (int nDebug = 0; nDebug < totalToGenerate / debugSteps; ++nDebug) {
    Msg.Format("%lld µs, %d\n", debugTimes[nDebug], debugBits[nDebug]);
    Result = f_write(&file, Msg, Msg.GetLength(), &nBytesWritten);
    if (Result != FR_OK || nBytesWritten != Msg.GetLength()) {
      m_Logger.Write(FromKernel, LogError, "Write error (%d)", Result);
      result = FailedPartially;
      break;
    }
  }

  Msg.Format("\nTime needed: %lld µs\nTotal bits generated: %d\n",
             newUptime - start, totalGenerated);
  Result = f_write(&file, Msg, Msg.GetLength(), &nBytesWritten);
  if (Result != FR_OK || nBytesWritten != Msg.GetLength()) {
    m_Logger.Write(FromKernel, LogError, "Write error (%d)", Result);
    result = FailedPartially;
  }

  Result = f_close(&file);
  if (Result == FR_OK) {
    m_Logger.Write(FromKernel, LogNotice, "Successfully written debug data to %s!", cFileNameDebug);
  } else {
    m_Logger.Write(FromKernel, LogPanic, "Cannot close debug data file (%d)", Result);
    result = FailedPartially;
  }

  return result;
}

MeasurementResult CKernel::WriteLatencyRngTest2() {
  MeasurementResult result = Okay;

#define FILENAME MEM_NAME_SIMPLE "_%d_measure.log"
  const CString fileName = GetFreeFile(DRIVE FILENAME);
  const char* cFileName = fileName;
  m_Logger.Write(FromKernel, LogNotice, "Choosing bits file %s", cFileName);

  FIL file;

  constexpr int tries1 = 20;
  constexpr int tries2 = 8;

  constexpr u8 num1s[] = {0x00, 0xff, 0xaa, 0x55, 0x73, 0xfc, 0xc5, 0x1c, 0x9d, 0x4c};
  constexpr u8 num2s[] = {0xff, 0x00, 0x55, 0xaa, 0x73, 0x36, 0x29, 0x9f, 0x1b, 0xd8};
  constexpr int bytes = sizeof(num1s) / sizeof(u8);

#if MEM_CAN_BURN_OUT
  constexpr int burnt1[] = {1, 9022, 26978, 44054, 60772};
  constexpr int burntAmount1 = sizeof(burnt1) / sizeof(int);
  constexpr int burnt2[] = {6, 10990, 31987, 54833, 64198};
  constexpr int burntAmount2 = sizeof(burnt2) / sizeof(int);
#endif

  constexpr int sane1[] = {3609, 17625, 29463, 48071, 58244};
  constexpr int saneAmount1 = sizeof(sane1) / sizeof(int);
  constexpr int sane2[] = {7541, 24251, 36203, 49382, 60456};
  constexpr int saneAmount2 = sizeof(sane2) / sizeof(int);

#if MEM_CAN_BURN_OUT
  const auto burntTimes1 = new u64[burntAmount1 * bytes * tries1];
  const auto burntTimes2 = new u64[burntAmount2 * 256 * 256 * tries2];
#endif
  const auto saneTimes1 = new u64[saneAmount1 * bytes * tries1];
  const auto saneTimes2 = new u64[saneAmount2 * 256 * 256 * tries2];

#if MEM_CAN_BURN_OUT
  bool burntOut;
  for (const int addr : burnt1) {
    result = IsBurntOut(burntOut, addr);
    if (result != Okay) return result;
    if (!burntOut) {
      result = BurnOut(addr);
      if (result != Okay) return result;
    }
  }
  for (const int addr : burnt2) {
    result = IsBurntOut(burntOut, addr);
    if (result != Okay) return result;
    if (!burntOut) {
      result = BurnOut(addr);
      if (result != Okay) return result;
    }
  }
#endif

  int idx;
  u64 latency;
  unsigned nBytesWritten;
  CString Msg;

#if MEM_CAN_BURN_OUT
  idx = 0;
  for (const int addr : burnt1) {
    for (int j = 0; j < bytes; ++j) {
      for (int k = 0; k < tries1; ++k) {
        RandomWriteLatency(latency, addr, num1s[j], num2s[j]);
        burntTimes1[idx++] = latency;
      }
    }
  }
  m_Logger.Write(FromKernel, LogNotice, "Burnt done");
#endif

  idx = 0;
  for (const int addr : sane1) {
    for (int j = 0; j < bytes; ++j) {
      for (int k = 0; k < tries1; ++k) {
        RandomWriteLatency(latency, addr, num1s[j], num2s[j]);
        saneTimes1[idx++] = latency;
      }
    }
  }
  m_Logger.Write(FromKernel, LogNotice, "Sane done");

#if MEM_CAN_BURN_OUT
  idx = 0;
  for (const int addr : burnt2) {
    for (int num1 = 0; num1 < 256; ++num1) {
      for (int num2 = 0; num2 < 256; ++num2) {
        for (int k = 0; k < tries2; ++k) {
          RandomWriteLatency(latency, addr, num1, num2);
          burntTimes2[idx++] = latency;
        }
      }
    }
  }
  m_Logger.Write(FromKernel, LogNotice, "Burnt full done");
#endif

  idx = 0;
  for (const int addr : sane2) {
    for (int num1 = 0; num1 < 256; ++num1) {
      for (int num2 = 0; num2 < 256; ++num2) {
        for (int k = 0; k < tries2; ++k) {
          RandomWriteLatency(latency, addr, num1, num2);
          saneTimes2[idx++] = latency;
        }
      }
    }
  }
  m_Logger.Write(FromKernel, LogNotice, "Sane full done");

  FRESULT Result = f_open(&file, fileName, FA_WRITE | FA_CREATE_ALWAYS);
  if (Result != FR_OK) {
    m_Logger.Write(FromKernel, LogPanic, "Cannot create file: %s (%d)", cFileName, Result);
    result = FailedTotally;
  }

#if MEM_CAN_BURN_OUT
  idx = 0;
  for (const int addr : burnt1) {
    for (int j = 0; j < bytes; ++j) {
      for (int k = 0; k < tries1; ++k) {
        Msg.Format("B,%d,%d,%d,%lld\n", addr, num1s[j], num2s[j], burntTimes1[idx++]);
        Result = f_write(&file, Msg, Msg.GetLength(), &nBytesWritten);
        if (Result != FR_OK || nBytesWritten != Msg.GetLength()) {
          m_Logger.Write(FromKernel, LogError, "Write error (%d)", Result);
          result = FailedPartially;
          break;
        }
      }
    }
  }
  m_Logger.Write(FromKernel, LogNotice, "Burnt written");
#endif

  idx = 0;
  for (const int addr : sane1) {
    for (int j = 0; j < bytes; ++j) {
      for (int k = 0; k < tries1; ++k) {
        Msg.Format("S,%d,%d,%d,%lld\n", addr, num1s[j], num2s[j], saneTimes1[idx++]);
        Result = f_write(&file, Msg, Msg.GetLength(), &nBytesWritten);
        if (Result != FR_OK || nBytesWritten != Msg.GetLength()) {
          m_Logger.Write(FromKernel, LogError, "Write error (%d)", Result);
          result = FailedPartially;
          break;
        }
      }
    }
  }
  m_Logger.Write(FromKernel, LogNotice, "Sane written");

#if MEM_CAN_BURN_OUT
  idx = 0;
  for (const int addr : burnt2) {
    for (int num1 = 0; num1 < 256; ++num1) {
      for (int num2 = 0; num2 < 256; ++num2) {
        for (int k = 0; k < tries2; ++k) {
          Msg.Format("B,%d,%d,%d,%lld\n", addr, num1, num2, burntTimes2[idx++]);
          Result = f_write(&file, Msg, Msg.GetLength(), &nBytesWritten);
          if (Result != FR_OK || nBytesWritten != Msg.GetLength()) {
            m_Logger.Write(FromKernel, LogError, "Write error (%d)", Result);
            result = FailedPartially;
            break;
          }
        }
      }
    }
  }
  m_Logger.Write(FromKernel, LogNotice, "Burnt full written");
#endif

  idx = 0;
  for (const int addr : sane2) {
    for (int num1 = 0; num1 < 256; ++num1) {
      for (int num2 = 0; num2 < 256; ++num2) {
        for (int k = 0; k < tries2; ++k) {
          Msg.Format("S,%d,%d,%d,%lld\n", addr, num1, num2, saneTimes2[idx++]);
          Result = f_write(&file, Msg, Msg.GetLength(), &nBytesWritten);
          if (Result != FR_OK || nBytesWritten != Msg.GetLength()) {
            m_Logger.Write(FromKernel, LogError, "Write error (%d)", Result);
            result = FailedPartially;
            break;
          }
        }
      }
    }
  }
  m_Logger.Write(FromKernel, LogNotice, "Sane full written");

  Result = f_close(&file);
  if (Result == FR_OK) {
    m_Logger.Write(FromKernel, LogNotice, "Successfully written bits to %s!", cFileName);
  } else {
    m_Logger.Write(FromKernel, LogPanic, "Cannot close bits file (%d)", Result);
    result = FailedPartially;
  }

  return result;
}

MeasurementResult CKernel::BurnOutCells() {
  MeasurementResult result = Okay;

  // Same cells as in WriteLatencyRngTest2
  constexpr int sane[] = {3609, 17625, 29463, 48071, 58244, 7541, 24251, 36203, 49382, 60456};
  constexpr int burnt[] = {1, 9022, 26978, 44054, 60772, 6, 10990, 31987, 54833, 64198};

  bool burntOut;

  for (const int addr : sane) {
    result = IsBurntOut(burntOut, addr);
    if (result != Okay) return result;
    if (burntOut) {
      m_Logger.Write(FromKernel, LogNotice, "Cell %d burnt out, not good!", addr);
    } else {
      m_Logger.Write(FromKernel, LogNotice, "Cell %d sane", addr);
    }
  }

  for (const int addr : burnt) {
    result = IsBurntOut(burntOut, addr);
    if (result != Okay) return result;
    if (!burntOut) {
      result = BurnOut(addr);
      if (result != Okay) return result;
    }
    m_Logger.Write(FromKernel, LogNotice, "Cell %d burnt out", addr);
  }

  m_Logger.Write(FromKernel, LogNotice, "Burn out process complete");

  return result;
}
