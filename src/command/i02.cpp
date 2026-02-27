#include "i02.h"

CommandResult handleI02(DeviceState &state) {
  if (!state.hasLastConfig) {
    return {ERR_PIN, false, false, false, false, false, false};
  }

  state.activePin = state.lastConfigPin;
  state.sampleHz = state.lastConfigHz;
  state.runState = RunState::Sampling;
  state.sampleOverflow = false;

  return {nullptr, true, false, true, false, false, true};
}
