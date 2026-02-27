#pragma once

#include <cstdint>

enum class CommandType : uint8_t {
  Unknown = 0,
  I01 = 1,
  I02 = 2,
  I03 = 3,
  I114 = 4,
  I999 = 5
};

struct DecodedCommand {
  CommandType type;
  uint8_t pin;
  uint16_t freq;
};

DecodedCommand decodeCommandLine(const char *line);
