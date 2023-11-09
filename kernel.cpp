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

  // Mount file system
  if (f_mount(&m_FileSystem, DRIVE, 1) != FR_OK) {
    m_Logger.Write(FromKernel, LogPanic, "Cannot mount drive: %s", DRIVE);
  }

  WriteLatencyRngTest();

  IndicateStop();

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

u64 CKernel::GetClockTicksHiLo() {
  PeripheralEntry();

  u32 hi = read32(ARM_SYSTIMER_CHI);
  u32 lo = read32(ARM_SYSTIMER_CLO);

  // double check hi value didn't change after setting it...
  if (hi != read32(ARM_SYSTIMER_CHI)) {
    hi = read32(ARM_SYSTIMER_CHI);
    lo = read32(ARM_SYSTIMER_CLO);
  }

  PeripheralExit();

  return static_cast<u64>(hi) << 32 | lo;
}

void CKernel::IndicateStop() {
  m_ActLED.Blink(1000000000);
}

u64 CKernel::RandomWriteLatency() {
  // These could also be fixed

  // Use HW RNG as "seed"
  /*int addr = (int) m_Random.GetNumber() % 512;
  int num1 = (int) m_Random.GetNumber() % 256;
  int num2 = (int) m_Random.GetNumber() % 256;*/

  // Use MT19937AR as "seed"
  const int addr = static_cast<int>(genrand_range(0, MEM_SIZE_ADR));
  const int num1 = static_cast<int>(genrand_range(0, 256));
  const int num2 = static_cast<int>(genrand_range(0, 256));

  // Write first value
  MemWrite(addr, num1);

  // Wait until the WEL latch turns reset
  WIPPollingCycles();

  // Overwrite value
  MemWrite(addr, num2);

  // Measure write latency
  const u64 write_latency = WIPPollingCycles();

  // This is the rather random latency
  return write_latency;
}

bool CKernel::WriteLatencyRandomBit() {
  // Extract "random" LSB
  return static_cast<bool>(RandomWriteLatency() & 1);
}

void CKernel::WriteLatencyRngTest() {
  int idxBits = 0, idxDebug = 0;
  u64 newUptime;

  constexpr int totalToGenerate = 500000;
  constexpr int debugSteps = 10000;

  char generated[totalToGenerate];
  u64 debugTimes[totalToGenerate / debugSteps];
  int debugBits[totalToGenerate / debugSteps];

  int toGenerate = totalToGenerate;
  int totalGenerated = 0;
  const u64 start = GetClockTicksHiLo();
  u64 blockStart = start;
  int blockGenerated = toGenerate;
  while (toGenerate > 0) {
    // Very basic implementation of von Neumann extractor
    const bool bit1 = WriteLatencyRandomBit();
    const bool bit2 = WriteLatencyRandomBit();
    totalGenerated += 2;
    if (bit1 == bit2) continue;
    // For more debug information:
    if (toGenerate % debugSteps == 0) {
      if (toGenerate < totalToGenerate) {
        newUptime = GetClockTicksHiLo();
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
    generated[idxBits] = static_cast<char>('0' + bit1);
    ++idxBits;
    --toGenerate;
  }

  newUptime = GetClockTicksHiLo();
  debugTimes[idxDebug] = newUptime - blockStart;
  debugBits[idxDebug] = totalGenerated - blockGenerated;
  CLogger::Get()->Write(FromKernel, LogNotice, "%lld µs, %d",
                        debugTimes[idxDebug], debugBits[idxDebug]);

  CLogger::Get()->Write(FromKernel, LogNotice, "Time needed: %lld µs", newUptime - start);
  CLogger::Get()->Write(FromKernel, LogNotice, "Total bits generated: %d\n", totalGenerated);

#define FILENAME_BITS "bits_%d.log"
  FIL File;
  CString fileName = GetFreeFile(DRIVE FILENAME_BITS);
  FRESULT Result = f_open(&File, fileName, FA_WRITE | FA_CREATE_ALWAYS);
  if (Result != FR_OK) {
    m_Logger.Write(FromKernel, LogPanic, "Cannot create file: %s (%d)", fileName, Result);
  }
  unsigned nBytesWritten;
  Result = f_write(&File, generated, totalToGenerate, &nBytesWritten);
  if (Result != FR_OK || nBytesWritten != totalToGenerate) {
    m_Logger.Write(FromKernel, LogError, "Write error (%d)", Result);
  }
  Result = f_close(&File);
  if (Result == FR_OK) {
    m_Logger.Write(FromKernel, LogNotice, "Successfully written bits to SD card!");
  } else {
    m_Logger.Write(FromKernel, LogPanic, "Cannot close bits file (%d)", Result);
  }

#define FILENAME_DEBUG "debug_%d.log"
  fileName = GetFreeFile(DRIVE FILENAME_DEBUG);
  Result = f_open(&File, fileName, FA_WRITE | FA_CREATE_ALWAYS);
  if (Result != FR_OK) {
    m_Logger.Write(FromKernel, LogPanic, "Cannot create file: %s (%d)", fileName, Result);
  }
  CString Msg;
  for (int nDebug = 0; nDebug < totalToGenerate / debugSteps; ++nDebug) {
    Msg.Format("%lld µs, %d\n", debugTimes[nDebug], debugBits[nDebug]);
    Result = f_write(&File, Msg, Msg.GetLength(), &nBytesWritten);
    if (Result != FR_OK || nBytesWritten != Msg.GetLength()) {
      m_Logger.Write(FromKernel, LogError, "Write error (%d)", Result);
      break;
    }
  }

  Msg.Format("\nTime needed: %lld µs\nTotal bits generated: %d\n",
             newUptime - start, totalGenerated);
  Result = f_write(&File, Msg, Msg.GetLength(), &nBytesWritten);
  if (Result != FR_OK || nBytesWritten != Msg.GetLength()) {
    m_Logger.Write(FromKernel, LogError, "Write error (%d)", Result);
  }

  Result = f_close(&File);
  if (Result == FR_OK) {
    m_Logger.Write(FromKernel, LogNotice, "Successfully written debug data to SD card!");
  } else {
    m_Logger.Write(FromKernel, LogPanic, "Cannot close debug data file (%d)", Result);
  }
}
