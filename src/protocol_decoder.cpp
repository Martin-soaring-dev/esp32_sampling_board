#include "protocol_decoder.h"

#include "configuration.h"
#include <cstdio>
#include <cstring>

namespace {
void normalizeCommand(const char *input, char *out, size_t outSize) {
  if (outSize == 0) {
    return;
  }
  out[0] = '\0';
  if (input == nullptr) {
    return;
  }

  strncpy(out, input, outSize);
  out[outSize - 1] = '\0';

  size_t len = strlen(out);
  while (len > 0 && (out[len - 1] == ' ' || out[len - 1] == '\t' || out[len - 1] == '\r' || out[len - 1] == '\n')) {
    out[--len] = '\0';
  }

  while (len >= 2 && out[len - 2] == '\\' && (out[len - 1] == 'r' || out[len - 1] == 'n')) {
    len -= 2;
    out[len] = '\0';
    while (len > 0 && (out[len - 1] == ' ' || out[len - 1] == '\t')) {
      out[--len] = '\0';
    }
  }

  size_t start = 0;
  while (out[start] == ' ' || out[start] == '\t') {
    start++;
  }
  if (start > 0) {
    memmove(out, out + start, strlen(out + start) + 1);
  }
}
} // namespace

DecodedCommand decodeCommandLine(const char *line) {
  DecodedCommand cmd {CommandType::Unknown, 0, 0};
  char normalized[CMD_BUFFER_SIZE];
  normalizeCommand(line, normalized, sizeof(normalized));

  if (strcmp(normalized, "I02") == 0) {
    cmd.type = CommandType::I02;
    return cmd;
  }

  if (strcmp(normalized, "I03") == 0) {
    cmd.type = CommandType::I03;
    return cmd;
  }

  if (strcmp(normalized, "I114") == 0) {
    cmd.type = CommandType::I114;
    return cmd;
  }

  if (strcmp(normalized, "I999") == 0) {
    cmd.type = CommandType::I999;
    return cmd;
  }

  int d = -1;
  int s = -1;
  int parsed = sscanf(normalized, "I01 D%d S%d", &d, &s);
  if (parsed == 2 && d >= 0 && d <= 255 && s >= 0 && s <= 65535) {
    cmd.type = CommandType::I01;
    cmd.pin = static_cast<uint8_t>(d);
    cmd.freq = static_cast<uint16_t>(s);
  }

  return cmd;
}
