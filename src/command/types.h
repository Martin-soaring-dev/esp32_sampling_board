#pragma once

#include "../configuration.h"
#include "../protocol_decoder.h"

struct DeviceState {
  uint8_t activePin;
  uint16_t sampleHz;
  uint8_t lastConfigPin;
  uint16_t lastConfigHz;
  uint32_t samplingStartMs;
  uint16_t lastVoltageMv;
  bool hasLastConfig;
  bool sampleOverflow;
  RunState runState;
};

struct CommandResult {
  const char *errorMsg;
  bool shouldStartSampling;
  bool shouldStopSampling;
  bool shouldClearBuffer;
  bool shouldReportStatus;
  bool shouldReboot;
  bool shouldResetTimestamp;
};

inline constexpr CommandResult COMMAND_NOOP {nullptr, false, false, false, false, false, false};
