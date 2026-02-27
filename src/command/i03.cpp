#include "i03.h"

CommandResult handleI03(DeviceState &state) {
  state.runState = RunState::Idle;
  return {nullptr, false, true, true, false, false, false};
}
