#include "dispatcher.h"

#include "i01.h"
#include "i02.h"
#include "i03.h"
#include "i114.h"
#include "i999.h"

CommandResult dispatchCommand(const DecodedCommand &cmd, DeviceState &state) {
  switch (cmd.type) {
    case CommandType::I01:
      return handleI01(cmd, state);
    case CommandType::I02:
      return handleI02(state);
    case CommandType::I03:
      return handleI03(state);
    case CommandType::I114:
      return handleI114();
    case CommandType::I999:
      return handleI999(state);
    case CommandType::Unknown:
    default:
      return COMMAND_NOOP;
  }
}
