#include "i01.h"

namespace {
bool isAnalogCapablePin(uint8_t pin) {
  // ESP32 analog-capable pins used here:
  // ADC1: GPIO:32,33,34,35,36,39
  // ADC2: GPIO:2,4,12,13,14,25,26,27 (enabled per project requirement)
  switch (pin) {
    case 2:
    case 4:
    case 12:
    case 13:
    case 14:
    case 25:
    case 26:
    case 27:
    case 32:
    case 33:
    case 34:
    case 35:
    case 36:
    case 39:
      return true;
    default:
      return false;
  }
}
} // namespace

CommandResult handleI01(const DecodedCommand &cmd, DeviceState &state) {
  if (!isAnalogCapablePin(cmd.pin)) {
    return {ERR_PIN, false, false, false, false, false, false};
  }
  if (cmd.freq < MIN_SAMPLE_HZ || cmd.freq > MAX_SAMPLE_HZ) {
    return {ERR_FREQ, false, false, false, false, false, false};
  }

  state.activePin = cmd.pin;
  state.sampleHz = cmd.freq;
  state.lastConfigPin = cmd.pin;
  state.lastConfigHz = cmd.freq;
  state.hasLastConfig = true;
  state.runState = RunState::Sampling;
  state.sampleOverflow = false;

  return {nullptr, true, false, true, false, false, false};
}
