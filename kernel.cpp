//
// kernel.cpp
//
#include "kernel.h"
#include "mt19937ar.h"

#define DRIVE        "SD:"

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
  m_Logger.Write(FromKernel, LogNotice, "Memory: %s, SPI Frequency: %lld", MEM_NAME, SPI_FREQ);

  // Do dummy measurement
  int raw = 0;
  bool bit;
  MeasurementResult result = ExtractSingleBit(bit, raw, 1000, 1000);
  if (result == Okay) {
    m_Logger.Write(FromKernel, LogNotice, "Successfully generated %d with %d raw bits!", bit, raw);
    m_ActLED.Blink(5, 100, 100);
  } else {
    m_Logger.Write(FromKernel, LogNotice, "Failed to generate single bit... Shutting down...");
    IndicateStop(result);
    return ShutdownNone;
  }

  // Mount file system
  if (f_mount(&m_FileSystem, DRIVE, 1) != FR_OK) {
    m_Logger.Write(FromKernel, LogPanic, "Cannot mount drive: %s", DRIVE);
    IndicateStop(FailedTotally);
  } else {
    // Do measurements only if successful
    result = WriteLatencyRngTest();
    IndicateStop(result);
  }

  return ShutdownNone;
}

bool CKernel::FileExists(const char* path) {
  return f_stat(path, nullptr) == FR_OK;
}

CString CKernel::GetFreeFile(const char* pattern) {
  CString Msg;
  int i = 0;
  while (true) {
    Msg.Format(pattern, i);
    if (!FileExists(Msg)) return Msg;
    ++i;
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

MeasurementResult CKernel::RandomWriteLatency(u64& write_latency, int timeout) {
  // These could also be fixed

  // Use HW RNG as "seed"
  /*const int addr = static_cast<int>(m_Random.GetNumber() % MEM_SIZE_ADR);
  const int num1 = static_cast<int>(m_Random.GetNumber() % 256);
  const int num2 = static_cast<int>(m_Random.GetNumber() % 256);*/

  // Use MT19937AR as "seed"
  const int addr = static_cast<int>(genrand_range(0, MEM_SIZE_ADR));
  const int num1 = static_cast<int>(genrand_range(0, 256));
  const int num2 = static_cast<int>(genrand_range(0, 256));

  // Write first value
  MemWrite(addr, num1);

  // Wait until the WIP bit is clear
  MeasurementResult result = WIPPollingCycles(write_latency);
  if (result != Okay) return result;

  // Overwrite value
  MemWrite(addr, num2);

  // write_latency should be rather random now
  result = WIPPollingCycles(write_latency);
  return result;
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
        CLogger::Get()->Write(FromKernel, LogNotice, "%lld µs, %d",
                              debugTimes[idxDebug], debugBits[idxDebug]);
        blockStart = newUptime;
        ++idxDebug;
      }
      blockGenerated = totalGenerated;
    }
    //CLogger::Get()->Write(FromMeasure, LogNotice, "%d", bit1);
    generated[totalToGenerate - toGenerate] = static_cast<char>('0' + bit);
    --toGenerate;
  }

  newUptime = CTimer::GetClockTicks64();
  debugTimes[idxDebug] = newUptime - blockStart;
  debugBits[idxDebug] = totalGenerated - blockGenerated;
  CLogger::Get()->Write(FromKernel, LogNotice, "%lld µs, %d",
                        debugTimes[idxDebug], debugBits[idxDebug]);

  CLogger::Get()->Write(FromKernel, LogNotice, "Time needed: %lld µs", newUptime - start);
  CLogger::Get()->Write(FromKernel, LogNotice, "Total bits generated: %d\n", totalGenerated);

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
