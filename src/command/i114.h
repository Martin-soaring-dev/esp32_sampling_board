#pragma once

#include "types.h"

struct I114StatusSnapshot {
  uint32_t serialBaud;
  size_t cmdBufferSize;
  size_t cmdBufferWriteIdx;
  bool cmdBufferOverflow;
  size_t sampleRingCapacity;
  uint16_t sampleRingUsed;
  uint16_t sampleRingFree;
  bool sampleRingOverflow;
  uint8_t activePin;
  uint16_t samplingHz;
  bool hasLastConfig;
  uint8_t lastPin;
  uint16_t lastSamplingHz;
  RunState runState;
  uint16_t pinMv;
  uint32_t workTimeMs;
};

CommandResult handleI114();
void emitI114StatusReport(const I114StatusSnapshot &snapshot);
