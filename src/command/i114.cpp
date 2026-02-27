#include "i114.h"

#include <Arduino.h>

namespace {
const char *runStateToString(RunState state) {
  switch (state) {
    case RunState::Idle:
      return "Idle";
    case RunState::Sampling:
      return "Sampling";
    case RunState::Stopping:
      return "Stopping";
    default:
      return "Unknown";
  }
}
} // namespace

CommandResult handleI114() {
  return {nullptr, false, false, false, true, false, false};
}

void emitI114StatusReport(const I114StatusSnapshot &s) {
  Serial.println("=== I114 SYSTEM STATUS ===");
  Serial.println("[SERIAL]");
  Serial.print("baud=");
  Serial.println(s.serialBaud);
  Serial.print("cmd_buffer_size=");
  Serial.println(s.cmdBufferSize);
  Serial.print("cmd_buffer_write_idx=");
  Serial.println(s.cmdBufferWriteIdx);
  Serial.print("cmd_buffer_overflow=");
  Serial.println(s.cmdBufferOverflow ? 1 : 0);
  Serial.println("[BUFFER]");
  Serial.print("sample_ring_capacity=");
  Serial.println(s.sampleRingCapacity);
  Serial.print("sample_ring_used=");
  Serial.println(s.sampleRingUsed);
  Serial.print("sample_ring_free=");
  Serial.println(s.sampleRingFree);
  Serial.print("sample_ring_overflow=");
  Serial.println(s.sampleRingOverflow ? 1 : 0);
  Serial.println("[PIN]");
  Serial.print("active_pin=GPIO");
  Serial.println(s.activePin);
  Serial.print("sampling_hz=");
  Serial.println(s.samplingHz);
  Serial.print("has_last_config=");
  Serial.println(s.hasLastConfig ? 1 : 0);
  Serial.print("last_pin=GPIO");
  Serial.println(s.lastPin);
  Serial.print("last_sampling_hz=");
  Serial.println(s.lastSamplingHz);
  Serial.print("run_state=");
  Serial.println(runStateToString(s.runState));
  Serial.println("[PIN VALUE & TIME]");
  Serial.print("pin_mv=");
  Serial.println(s.pinMv);
  Serial.print("work_time_ms=");
  Serial.println(s.workTimeMs);
  Serial.println("=== END STATUS ===");
}
