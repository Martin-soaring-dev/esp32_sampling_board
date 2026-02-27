#include "i999.h"

#include <Arduino.h>

CommandResult handleI999(DeviceState &state) {
  state.runState = RunState::Idle;
  return {nullptr, false, true, true, false, true, false};
}

void executeI999Reboot() {
  Serial.println("I999 rebooting...");
  Serial.flush();
  delay(50);
  ESP.restart();
}
